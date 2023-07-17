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
  const auto & mg = _app.getMeshGenerator(mesh_generator_name);
}

void
MonteCarloGeomAction::makeAssemblyMeshJSON(std::string mesh_generator_name)
{
  const auto & mg = _app.getMeshGenerator(mesh_generator_name);
}

void
MonteCarloGeomAction::makePinMeshJSON(std::string mesh_generator_name)
{
  const auto & mg = _app.getMeshGenerator(mesh_generator_name);

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