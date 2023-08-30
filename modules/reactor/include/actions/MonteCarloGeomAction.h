//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#pragma once

#include "Action.h"
#include "nlohmann/json.h"

class MonteCarloGeomAction : public Action
{
public:
  MonteCarloGeomAction(const InputParameters & params);

  static InputParameters validParams();

  virtual void act() override;

  nlohmann::json makeCoreMeshJSON(std::string mesh_generator_name, std::string rpm_name);
  nlohmann::json makeAssemblyMeshJSON(std::string mesh_generator_name, std::string rpm_name);
  nlohmann::json makePinMeshJSON(std::string mesh_generator_name, std::string rpm_name);
  nlohmann::json makeLattice(std::string geom_mesh_type,
                             std::vector<std::vector<int>> lattice,
                             std::vector<std::string> names,
                             std::string unit_name,
                             std::string fill_name,
                             int axial_id,
                             Real null_height);
};