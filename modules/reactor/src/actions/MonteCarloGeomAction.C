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
    }

  }
}

