//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "CSGBase.h"

CSGBase::CSGBase()
{
  sample_num = 2;
}

void
CSGBase::example_func()
{
  std::cout << "Calling example csg method \n";
}