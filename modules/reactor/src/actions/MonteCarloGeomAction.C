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
#include "MooseObjectAction.h"

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

  // get prefix for all data
  auto & pin_type = mg.getParam<subdomain_id_type>("pin_type");
  bool is_assem = mg.getParam<bool>("use_as_assembly");
  std::string mg_struct = is_assem ? "assembly" : "pin";
  std::string prefix = mg_struct + "_" + std::to_string(pin_type);

  // get data
  Real pitch = getMeshProperty<Real>(prefix + "_pitch", mesh_generator_name);

  Moose::out << "pin pitch for " << mesh_generator_name << " : " << prefix << " " << pitch << std::endl;

  // mg.hasMeshProperty(str)
  // mg.getMeshProperty(str)
}