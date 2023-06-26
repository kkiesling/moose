//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "AddMonteCarloGeomAction.h"
#include "Factory.h"
#include "Parser.h"
#include "FEProblem.h"

registerMooseAction("ReactorApp", AddMonteCarloGeomAction, "create_pin");
registerMooseAction("ReactorApp", AddMonteCarloGeomAction, "create_assembly");
registerMooseAction("ReactorApp", AddMonteCarloGeomAction, "create_core");

InputParameters
AddMonteCarloGeomAction::validParams()
{
  InputParameters params = Action::validParams();
  return params;
}

AddMonteCarloGeomAction::AddMonteCarloGeomAction(const InputParameters & params) : Action(params) {}

void
AddMonteCarloGeomAction::act()
{
  if (_current_task == "create_pin")
  {
    //pass
  }
}