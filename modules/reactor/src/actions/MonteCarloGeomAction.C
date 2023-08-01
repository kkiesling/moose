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
      // gather list of units
      nlohmann::json titan_inp;

      // final mesh generator to use
      const auto & final_mg = _app.getMeshGenerator(final_mgn);

      for (const auto & mgn : mg_names)
      {
        // only generate inputs for mesh generators that are parents
        // to the final requested
        if (final_mg.isParentMeshGenerator(mgn, false) || (mgn == final_mgn))
        {
          nlohmann::json units;
          const auto & mg = _app.getMeshGenerator(mgn);
          if (mg.type() == "CoreMeshGenerator")
          {
            titan_inp["units"].update(makeCoreMeshJSON(mgn, rpm_name));
          }
          else if (mg.type() == "AssemblyMeshGenerator")
          {
            titan_inp["units"].update(makeAssemblyMeshJSON(mgn, rpm_name));
          }
          else if (mg.type() == "PinMeshGenerator")
          {
            titan_inp["units"].update(makePinMeshJSON(mgn, rpm_name));
          }
        }
      }

    Moose::out << titan_inp.dump(4) << std::endl;

    }
  }
}

nlohmann::json
MonteCarloGeomAction::makeLattice(std::string geom_mesh_type,
                                  std::vector<std::vector<int>> lattice,
                                  std::vector<std::string> names,
                                  std::string unit_name,
                                  std::string fill_name,
                                  int axial_id)
{
  nlohmann::json titan_inp;

  // check if square or hex lattice and get dimension
  std::string lat_type;
  int dimension;
  std::string dim_type;
  if (geom_mesh_type == "Hex")
  {
    lat_type = "HEX_MAP";
    dimension = lattice.size() / 2 + 1; // integer division
    dim_type = "num_rings";
  }
  else
  {
    lat_type = "SQUARE_MAP";
    dimension = lattice[0].size();
    dim_type = "dim";
  }

  // iterate over elements
  bool make_null = false;
  std::vector<std::vector<std::string>> elements;
  for (auto & row_list : lattice)
  {
    std::vector<std::string> ele_list; // ring or row element fills
    for (auto & ele_id : row_list)
    {
      std::string ele_name;
      if (ele_id == -1)
      {
          ele_name = unit_name + "_null";
          make_null = true;
      }
      else
      {
          ele_name = names[ele_id];
      }
      // indicate axial region in name
      if (axial_id > -1)
      {
        ele_name = ele_name + "_axial_" + std::to_string(axial_id);
      }
      ele_list.push_back(ele_name);
      }
    elements.push_back(ele_list);
  }

  // get set of pitches, if more than one pitch, issue warning and use largest
  std::vector<Real> pitches;
  for (auto & ele_name : names)
  {
    // get pitch associated with element
    const auto assem_pitch = getMeshProperty<Real>(RGMB::pitch, ele_name);
    pitches.push_back(assem_pitch);
  }
  std::set<Real> set_pitches(pitches.begin(), pitches.end());
  Real max_pitch = *std::max_element(pitches.begin(), pitches.end());
  if (set_pitches.size() > 1)
  {
    mooseWarning(unit_name + " has " + std::to_string(set_pitches.size()) +
                 " associated with the lattice. Using largest: " + std::to_string(max_pitch));
  }

  // form lattice
  titan_inp[unit_name] = {lat_type,
                          {{dim_type, dimension},
                          {"pitch", max_pitch},
                          {"fill", fill_name},
                          {"elements", elements}}};

  // if lattice has dummy assembly, make a dummy assembly
  if (make_null)
  {
    int num_sides;
    if (lat_type == "HEX_MAP")
    {
    num_sides = 6;
    }
    else
    {
    num_sides = 4;
    }

    titan_inp[unit_name + "_null"] = {"POLYGON_DOMAIN", fill_name, {num_sides, max_pitch / 2.0}};
  }

  return titan_inp;
}

nlohmann::json
MonteCarloGeomAction::makeCoreMeshJSON(std::string mesh_generator_name, std::string rpm_name)
{
  nlohmann::json titan_inp;
  const auto geom_mesh_type = getMeshProperty<std::string>(RGMB::mesh_geometry, rpm_name);

  // get assembly lattice
  const auto assembly_lattice =
      getMeshProperty<std::vector<std::vector<int>>>(RGMB::assembly_lattice, mesh_generator_name);
  const auto assembly_names =
      getMeshProperty<std::vector<std::string>>(RGMB::assembly_names, mesh_generator_name);

  titan_inp = makeLattice(geom_mesh_type, assembly_lattice, assembly_names, mesh_generator_name, "void", -1);

  return titan_inp;
}

nlohmann::json
MonteCarloGeomAction::makeAssemblyMeshJSON(std::string mesh_generator_name, std::string rpm_name)
{
  nlohmann::json titan_inp;

  // determine if 3d or 2d geom and get axial heights
  int geom_dim = getMeshProperty<int>(RGMB::mesh_dimensions, rpm_name);
  const auto geom_mesh_type = getMeshProperty<std::string>(RGMB::mesh_geometry, rpm_name);
  std::vector<Real> axial_heights;
  if (geom_dim == 3)
  {
    axial_heights = getMeshProperty<std::vector<Real>>(RGMB::axial_mesh_sizes, rpm_name);
  }

  // get pin lattice
  const auto pin_lattice =
      getMeshProperty<std::vector<std::vector<int>>>(RGMB::pin_lattice, mesh_generator_name);
  const auto pin_names =
      getMeshProperty<std::vector<std::string>>(RGMB::pin_names, mesh_generator_name);

  // iterate over each axial region and make lattice for each
  int ax_id = 0;
  std::vector<std::pair<std::string, Real>> axial_stack; // list of axial region names to compile into axial stack
  const auto bg_region_ids = getMeshProperty<std::vector<unsigned short>>(
      RGMB::background_region_id, mesh_generator_name);
  for (auto & ax_reg : bg_region_ids)
  {
    std::string bg_material = "material_" + std::to_string(ax_reg);

    // get unit name
    std::string unit_name;
    if (geom_dim == 3)
    {
      unit_name = mesh_generator_name + "_lattice_axial_" + std::to_string(ax_id);
    }
    else
    {
      unit_name = mesh_generator_name + "_lattice";
    }

    // make the lattice
    titan_inp[unit_name] = makeLattice(geom_mesh_type, pin_lattice, pin_names, unit_name, bg_material, ax_id);

    // get height from axial heights list and name region accordingly
    if (geom_dim == 3)
    {
      // get height from axial heights list and name region accordingly
      std::pair<std::string, Real> p;
      p.first = unit_name;
      p.second = axial_heights[ax_id];
      axial_stack.push_back(p);
    }
    ax_id = ax_id + 1;
  }

  // get any duct regions and insert them into each other
  if (hasMeshProperty(RGMB::duct_halfpitches, mesh_generator_name))
  {
    int num_sides;
    if (geom_mesh_type == "Hex")
    {
      num_sides = 6;
    }
    else{
      num_sides = 4;
    }

    // iterate over duct regions (assume half pitches are in ascending order)
    const auto duct_halfpitches = getMeshProperty<std::vector<Real>>(RGMB::duct_halfpitches, mesh_generator_name);
    const auto duct_region_ids = getMeshProperty<std::vector<std::vector<unsigned short>>>(RGMB::duct_region_ids, mesh_generator_name);
    Real max_pitch = *std::max_element(duct_halfpitches.begin(), duct_halfpitches.end());

    // reset the axial stack and use the nested ducted regions for the stack instead
    axial_stack.clear() ;

    int ax_id = 0;
    for (auto & ax_reg : duct_region_ids)
    {
      std::string last_unit_name;
      int duct_id = 0;
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

        // if 3d append axial id
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

  return titan_inp;
}

nlohmann::json
MonteCarloGeomAction::makePinMeshJSON(std::string mesh_generator_name, std::string rpm_name)
{
  nlohmann::json titan_inp;
  const auto is_single_pin = getMeshProperty<bool>(RGMB::is_single_pin, mesh_generator_name);
  int geom_dim = getMeshProperty<int>(RGMB::mesh_dimensions, rpm_name);
  const auto geom_mesh_type = getMeshProperty<std::string>(RGMB::mesh_geometry, rpm_name);

  std::vector<Real> axial_heights;
  if (geom_dim == 3)
  {
    axial_heights = getMeshProperty<std::vector<Real>>(RGMB::axial_mesh_sizes, rpm_name);
  }


  // make radial pins
  if (!is_single_pin)
  {
    // determine if 2d or 3d (extruded for 3d)
    // get some basic parameters for entire geometry from RMP
    std::string geom_type = "PIN";


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
  }
  else{
    // make a filled assembly
    int num_sides;
    if (geom_mesh_type == "Hex")
    {
      num_sides = 6;
    }
    else
    {
      num_sides = 4;
    }

    const auto pitch = getMeshProperty<Real>(RGMB::pitch, mesh_generator_name);

    // iterate over each axial region and make lattice for each
    int ax_id = 0;
    std::vector<std::pair<std::string, Real>>
        axial_stack; // list of axial region names to compile into axial stack
    const auto bg_region_ids = getMeshProperty<std::vector<unsigned short>>(
        RGMB::background_region_id, mesh_generator_name);
    for (auto & ax_reg : bg_region_ids)
    {
      std::string material = "material_" + std::to_string(ax_reg);

      // get height from axial heights list and name region accordingly
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

      // form lattice for this axial region
      titan_inp[unit_name] = {"POLYGON_DOMAIN", material, {num_sides, pitch / 2.0}};
      ax_id = ax_id + 1;
    }

    if (geom_dim == 3)
    {
      titan_inp[mesh_generator_name] = {"AXIAL_STACK", axial_stack};
    }
  }

  return titan_inp;
}

