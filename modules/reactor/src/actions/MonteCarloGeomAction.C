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
#include "nlohmann/json.h"

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
    for (const auto & mgn : mg_names)
    {
      const auto & mg = _app.getMeshGenerator(mgn);
      if (mg.type() == "ReactorMeshParams"){
        make_mc = mg.isParamValid("generate_mc_geometry") ? mg.getParam<bool>("generate_mc_geometry") : false;
        break;
      }
    }

    // make geometry
    if (make_mc)
    {
      // make the titan input
      Moose::out << "We are making the Titan input" << std::endl;
      //auto & output_mesh = _app.actionWarehouse().mesh()->getMesh();

      for (const auto & mgn : mg_names)
      {
        const auto & mg = _app.getMeshGenerator(mgn);
        //Moose::out << "Mesh Generator: " << mgn << " " << mg.type() << std::endl;

        if (mg.type() == "CoreMeshGenerator")
        {
          makeCoreMeshJSON(mgn);
        }
        else if (mg.type() == "AssemblyMeshGenerator")
        {
          makeAssemblyMeshJSON(mgn);
        }
        else if (mg.type() == "PinMeshGenerator")
        {
          makePinMeshJSON(mgn); // if use as assembly, then need to call assembly
        }

      }
    }

  }
}

void
MonteCarloGeomAction::makeCoreMeshJSON(std::string mesh_generator_name)
{
  //const auto & mg = _app.getMeshGenerator(mesh_generator_name);
}

void
MonteCarloGeomAction::makeAssemblyMeshJSON(std::string mesh_generator_name)
{
  //const auto & mg = _app.getMeshGenerator(mesh_generator_name);
  nlohmann::json titan_inp;

  const auto is_single_pin = getMeshProperty<bool>(RGMB::is_single_pin, mesh_generator_name);

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

    // convert list of ints to corresponding list of pin names
    std::vector<std::vector<std::string>> elements;
    for (auto & pin_list : pin_lattice)
    {
      std::vector<std::string> ele_list;  // ring or row element fills

      for (auto & pin_id : pin_list)
      {
        std::string pin_name = pin_names[pin_id];
        ele_list.push_back(pin_name);
      }
      elements.push_back(ele_list);
    }

    titan_inp[mesh_generator_name + "_lattice"] = {
      lat_type,
      {
        {dim_type, dimension},
        {"pitch", 1.0},  // placeholder
        {"fill", "mat"}, // placeholder
        {"elements", elements}
      }
    };

    // get any duct regions
    if (hasMeshProperty(RGMB::duct_halfpitches, mesh_generator_name))
    {
      int duct_id = 0;
      const auto duct_halfpitches = getMeshProperty<std::vector<Real>>(RGMB::duct_halfpitches, mesh_generator_name);

      for (auto & hp : duct_halfpitches)
      {
        // create a polygon with the appropriate inserts
      }
    }

    // create the actual assembly which is the combo of any lattice and duct regions
    // name for the full assembly is mesh_generator_name with no suffixes

    Moose::out << titan_inp.dump(4) << std::endl;

  }

}

void
MonteCarloGeomAction::makePinMeshJSON(std::string mesh_generator_name)
{
  //const auto & mg = _app.getMeshGenerator(mesh_generator_name);
  nlohmann::json titan_inp;

  // get the ring radii and create the pin/material list
  const auto radii = getMeshProperty<std::vector<Real>>(RGMB::ring_radii, mesh_generator_name);
  if (radii.size() > 0)
  {
    int i = 0;  // index for material name placeholder for each region
    std::vector<std::pair<std::string, Real>> radii_list;
    for(auto & r : radii)
    {
      std::string mat_name = mesh_generator_name + "_mat_" + std::to_string(i);
      std::pair<std::string, Real> p;
      p.first = mat_name;
      p.second = r;
      radii_list.push_back(p);
      i = i + 1;
    }
    titan_inp[mesh_generator_name] = {"PIN", radii_list};
  }

  Moose::out << titan_inp.dump(4) << std::endl;
}