//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "MonteCarloGeomAction.h"
#include "ReactorGeometryMeshBuilderBase.h"
#include "ReactorMeshParams.h"
#include "MeshGenerator.h"

registerMooseAction("ReactorApp", MonteCarloGeomAction, "make_mc");

InputParameters
MonteCarloGeomAction::validParams()
{
  InputParameters params = Action::validParams();
  return params;
}

MonteCarloGeomAction::MonteCarloGeomAction(const InputParameters & params) : Action(params) {}

void
MonteCarloGeomAction::act()
{
  if (_current_task == "make_mc")
  {
    // find ReactorMeshParams input and check if generate_mc_geometry is true
    bool make_mc = false;
    std::vector<std::string> mg_names = _app.getMeshGeneratorNames();
    std::string rpm_name;
    std::string final_mgn;
    for (const auto & mgn : mg_names)
    {
      const auto & mg = _app.getMeshGenerator(mgn);
      if (mg.type() == "ReactorMeshParams"){
        rpm_name = mgn;
        if (mg.isParamValid("mc_geometry"))
        {
          final_mgn = mg.getParam<std::string>("mc_geometry");
          const auto & final_mg = _app.getMeshGenerator(final_mgn);
          // check that mg is of correct type
          if ((final_mg.type() == "CoreMeshGenerator")
            || (final_mg.type() == "AssemblyMeshGenerator")
            || (final_mg.type() == "PinMeshGenerator"))
          {
            make_mc = true;
          }
          else
          {
            make_mc = false;
            mooseError("Monte Carlo CSG geometry requested for " + final_mgn + " which is not of type CoreMeshGenerator, AssemblyMeshGenerator, or PinMeshGenerator.");
          }
        }
        else{
          make_mc = false;
        }
        break;
      }
    }

    // make geometry
    if (make_mc)
    {
      // make the titan input
      Moose::out << "We are making the Titan input" << std::endl;

      // gather list of units
      std::vector<nlohmann::json> units;

      // final mesh generator to use
      const auto & final_mg = _app.getMeshGenerator(final_mgn);

      for (const auto & mgn : mg_names)
      {
        // only generate inputs for mesh generators that are parents
        // to the final requested
        if (final_mg.isParentMeshGenerator(mgn, false) || (mgn == final_mgn))
        {
          const auto & mg = _app.getMeshGenerator(mgn);
          if (mg.type() == "CoreMeshGenerator")
          {
            makeCoreMeshJSON(mgn, rpm_name);
          }
          else if (mg.type() == "AssemblyMeshGenerator")
          {
            units.push_back(makeAssemblyMeshJSON(mgn, rpm_name));
          }
          else if (mg.type() == "PinMeshGenerator")
          {
            units.push_back(makePinMeshJSON(mgn, rpm_name));
          }
        }
      }

    nlohmann::json titan_inp;
    titan_inp["units"] = units;
    Moose::out << titan_inp.dump(4) << std::endl;

    }
  }
}

void
MonteCarloGeomAction::makeCoreMeshJSON(std::string mesh_generator_name, std::string rpm_name)
{
  //const auto & mg = _app.getMeshGenerator(mesh_generator_name);
}

nlohmann::json
MonteCarloGeomAction::makeAssemblyMeshJSON(std::string mesh_generator_name, std::string rpm_name)
{
  //const auto & mg = _app.getMeshGenerator(mesh_generator_name);
  nlohmann::json titan_inp;

  const auto is_single_pin = getMeshProperty<bool>(RGMB::is_single_pin, mesh_generator_name);

  // determine if 3d or 2d geom and get axial heights
  int geom_dim = getMeshProperty<int>(RGMB::mesh_dimensions, rpm_name);
  std::vector<Real> axial_heights;
  if (geom_dim == 3)
  {
    axial_heights = getMeshProperty<std::vector<Real>>(RGMB::axial_mesh_sizes, rpm_name);
  }

  if  (!is_single_pin)
  {
    // get pin lattice
    const auto pin_lattice =
        getMeshProperty<std::vector<std::vector<int>>>(RGMB::pin_lattice, mesh_generator_name);
    const auto pin_names =
        getMeshProperty<std::vector<std::string>>(RGMB::pin_names, mesh_generator_name);

    // check if square or hex lattice and get dimension
    std::string lat_type;
    int dimension;
    std::string dim_type;
    if (pin_lattice[0].size() == pin_lattice[1].size())
    {
      lat_type = "SQUARE_MAP";
      dimension = pin_lattice[0].size();
      dim_type = "dim";
    }
    else
    {
      lat_type = "HEX_MAP";
      dimension = pin_lattice.size() / 2 + 1; // integer division
      dim_type = "num_rings";
    }

    // iterate over each axial region and make lattice for each
    int ax_id = 0;
    std::vector<std::pair<std::string, Real>> axial_stack; // list of axial region names to compile into axial stack
    const auto bg_region_ids = getMeshProperty<std::vector<unsigned short>>(
        RGMB::background_region_id, mesh_generator_name);
    for (auto & ax_reg : bg_region_ids)
    {
      std::string bg_material = "material_" + std::to_string(ax_reg);

      // convert list of ints to corresponding list of pin names
      std::vector<std::vector<std::string>> elements;
      for (auto & pin_list : pin_lattice)
      {
        std::vector<std::string> ele_list;  // ring or row element fills
        for (auto & pin_id : pin_list)
        {
          std::string pin_name;
          if (geom_dim == 3) {
            // fill with only the corresponding axial region of the pin
            pin_name = pin_names[pin_id] + "_axial_" + std::to_string(ax_id);
          }
          else
          {
            pin_name = pin_names[pin_id];
          }
          ele_list.push_back(pin_name);
        }
        elements.push_back(ele_list);
      }

      // get set of pitches, if more than one pitch, issue warning and use largest
      std::vector<Real> pitches;
      for (auto & pin_name : pin_names)
      {
        // get pitch associated with pin
        const auto pin_pitch = getMeshProperty<Real>(RGMB::pitch, pin_name);
        pitches.push_back(pin_pitch);
      }
      std::set<Real> set_pitches(pitches.begin(), pitches.end());
      Real max_pitch = *std::max_element(pitches.begin(), pitches.end());
      if (set_pitches.size() > 1)
      {
        mooseWarning(mesh_generator_name + " has " + std::to_string(set_pitches.size())
          + " associated with the lattice. Using largest: " + std::to_string(max_pitch));
      }

      // get height from axial heights list and name region accordingly
      std::string unit_name;
      if (geom_dim == 3)
      {
        // get height from axial heights list and name region accordingly
        unit_name = mesh_generator_name + "_lattice_axial_" + std::to_string(ax_id);
        std::pair<std::string, Real> p;
        p.first = unit_name;
        p.second = axial_heights[ax_id];
        axial_stack.push_back(p);
      }
      else
      {
        unit_name = mesh_generator_name + "_lattice";
      }

      // form lattice for this axial region
      titan_inp[unit_name] = {
        lat_type,
        {
          {dim_type, dimension},
          {"pitch", max_pitch},
          {"fill", bg_material},
          {"elements", elements}
        }
      };
      ax_id = ax_id + 1;
    }

    // get any duct regions and insert them into each other
    if (hasMeshProperty(RGMB::duct_halfpitches, mesh_generator_name))
    {
      int num_sides;
      if (lat_type == "HEX_MAP")
      {
        num_sides = 6;
      }
      else{
        num_sides = 4;
      }

      // iterate over duct regions (assume half pitches are in ascending order)
      int duct_id = 0;
      const auto duct_halfpitches = getMeshProperty<std::vector<Real>>(RGMB::duct_halfpitches, mesh_generator_name);
      const auto duct_region_ids = getMeshProperty<std::vector<std::vector<unsigned short>>>(RGMB::duct_region_ids, mesh_generator_name);
      Real max_pitch = *std::max_element(duct_halfpitches.begin(), duct_halfpitches.end());

      int ax_id = 0;
      for (auto & ax_reg : duct_region_ids)
      {
        // reset the axial stack and use the nested ducted regions for the stack instead
        axial_stack.clear() ;

        std::string last_unit_name;
        for (auto & hp : duct_halfpitches)
        {
          std::string material = "material_" + std::to_string(ax_reg[duct_id]);

          // if we are working with the outermost ducted region and geometry is 2D,
          // then the unit name is the mesh generator name
          std::string unit_name;
          if ((hp == max_pitch) && (geom_dim == 2))
          {
            unit_name = mesh_generator_name;
          }
          else
          {
            // define base name by ducted region
            unit_name = mesh_generator_name + "_duct_" + std::to_string(duct_id);
          }

          // add axial region to name if 3d
          if (geom_dim == 3)
          {
            unit_name =  unit_name + "_axial_" + std::to_string(ax_id);
          }

          // get "inserts" for the polygon
          std::vector<std::string> inserts;
          std::string name_insert ;
          if (duct_id == 0)
          {
            // insert pin lattice for inner-most
            name_insert = mesh_generator_name + "_lattice";
          }
          else
          {
            // insert previous ducted region for all other regions
            name_insert = mesh_generator_name + "_duct_" + std::to_string(duct_id - 1);

          }
          if (geom_dim == 3){
            name_insert = name_insert + "_axial_" + std::to_string(ax_id);
          }

          // make the polygon with appropriate inserts
          inserts.push_back(name_insert);
          titan_inp[unit_name] = {"POLYGON_DOMAIN", material, {num_sides, hp}, {"inserts", inserts}};

          // record last radial region and use this as the stack info
          last_unit_name = unit_name;

          duct_id = duct_id + 1;
        }

        if (geom_dim == 3)
        {
          // record the axial stack unit
          std::pair<std::string, Real> p;
          p.first = last_unit_name;
          p.second = axial_heights[ax_id];
          axial_stack.push_back(p);
        }

        ax_id = ax_id + 1;
      }
    }

    // if working in 3D create the axial stack
    if (geom_dim == 3)
    {
      titan_inp[mesh_generator_name] = {"AXIAL_STACK", axial_stack};
    }
  }

  else{
    // get single_pin assembly info
  }
  return titan_inp;

}

nlohmann::json
MonteCarloGeomAction::makePinMeshJSON(std::string mesh_generator_name, std::string rpm_name)
{
  const auto is_single_pin = getMeshProperty<bool>(RGMB::is_single_pin, mesh_generator_name);
  if (is_single_pin)
  {
    // call assembly generator instead
    return makeAssemblyMeshJSON(mesh_generator_name, rpm_name);
  }

  //const auto & mg = _app.getMeshGenerator(mesh_generator_name);

  nlohmann::json titan_inp;

  // determine if 2d or 3d (extruded for 3d)
  // get some basic parameters for entire geometry from RMP
  int geom_dim = getMeshProperty<int>(RGMB::mesh_dimensions, rpm_name);
  std::string geom_type = "PIN";
  std::vector<Real> axial_heights;
  if (geom_dim == 3)
  {
    axial_heights = getMeshProperty<std::vector<Real>>(RGMB::axial_mesh_sizes, rpm_name);
  }

  // get the ring radii and create the pin/material list
  const auto radii = getMeshProperty<std::vector<Real>>(RGMB::ring_radii, mesh_generator_name);

  // get all axial regions
  const auto ring_region_ids = getMeshProperty<std::vector<std::vector<unsigned short>>>(RGMB::ring_region_ids, mesh_generator_name);

  // iterate over each axial region and make extruded pin for each
  int ax_id = 0;
  std::vector<std::pair<std::string, Real>> axial_stack; // list of axial region names to compile into axial stack
  for (auto & ax_reg : ring_region_ids)
  {
    // get the radial regions
    std::vector<std::pair<std::string, Real>> radii_list;
    int rad_id = 0;
    for (auto & r : radii)
    {
      // material for this radial region given by the list of ids for this axial region
      std::string mat_name = "material_" + std::to_string(ax_reg[rad_id]);
      std::pair<std::string, Real> p;
      p.first = mat_name;
      p.second = r;
      radii_list.push_back(p);
      rad_id = rad_id + 1;
    }

    // create this axial unit made of the above radial regions
    std::string unit_name;
    if (geom_dim == 3)
    {
      // get height from axial heights list and name region accordingly
      unit_name = mesh_generator_name + "_axial_" + std::to_string(ax_id);
      std::pair<std::string, Real> p;
      p.first = unit_name;
      p.second = axial_heights[ax_id];
      axial_stack.push_back(p);
    }
    else
    {
      unit_name = mesh_generator_name;
    }
    titan_inp[unit_name] = {geom_type, radii_list};
    ax_id = ax_id + 1;
  }

  // if working in 3D create the axial stack
  if (geom_dim == 3)
  {
    titan_inp[mesh_generator_name] = {"AXIAL_STACK", axial_stack};
  }

  return titan_inp;
}