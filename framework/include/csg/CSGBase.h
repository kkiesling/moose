//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#pragma once

#include "MooseObject.h"
#include "MooseApp.h"
#include "MooseMeshUtils.h"

class CSGBase // Complete CSG Object
{
    public:
        //static InputParameters validParams();
        //CSGBase(const InputParameters & parameters);
        CSGBase();
        void example_func();
        int sample_num;


};

class CSGType
{
  // generic CSG element/object type
  public:
    const std::string name;
    const std::int id;
    // transformations
    // figure out correct types of input to be consistent with MOOSE type of input
    // allow to be overrided depending on object type?
    virtual void rotate();
    virtual void translate();
};

class CSGSurface : public CSGType
c{
    // General CSG Surface type
    public:
        //std::string boundary_type = "transmission"; // do we need this?

        // coefficients that are used to define an arbitrary surface
        // initialized to 0
        struct Coeffs
        {
            Real a = 0.0;
            Real b = 0.0;
            Real c = 0.0;
            Real d = 0.0;
            Real r = 0.0;
            Real x = 0.0;
            Real y = 0.0;
            Real z = 0.0;
        };
        Coeffs coefficients;
        virtual std::vector<Real> getCoefficients(){
            //return coefficients;
            // return only relevant coefficients as a map (not a vector?)
        };
        virtual

};

class CSGPlane : public CSGSurface
{
    // Arbitrary Plane
    // Ax + By + Cz = D

    std::vector<Real> getCoefficients() override;
    void rotate() override;
    void translate() override;
};

class CSGCylinder : public CSGSurface
{
  // Arbitrary Cylinder
  //

  std::vector<Real> getCoefficients() override;
  void rotate() override;
  void translate() override;
};

class CSGCell : public CSGType
{
    // General CSG Cell type
    public:

}