//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "CSGSurfaceList.h"
#include "CSGPlane.h"
#include "CSGSphere.h"
#include "CSGXCylinder.h"
#include "CSGYCylinder.h"
#include "CSGZCylinder.h"

namespace CSG
{

CSGSurfaceList::CSGSurfaceList() {}

const CSGSurface &
CSGSurfaceList::getSurface(const std::string & name) const
{
  if (const auto it = _surfaces.find(name); it != _surfaces.end())
  {
    auto & surface_ptr = it->second;
    mooseAssert(surface_ptr, "Null surface");
    return *surface_ptr;
  }
  mooseError("No surface by name " + name + " exists in the geometry.");
}

CSGSurface &
CSGSurfaceList::getSurface(const std::string & name)
{
  return const_cast<CSGSurface &>(std::as_const(*this).getSurface(name));
}

std::vector<std::reference_wrapper<const CSGSurface>>
CSGSurfaceList::getAllSurfaces() const
{
  std::vector<std::reference_wrapper<const CSGSurface>> surfaces;
  for (auto it = _surfaces.begin(); it != _surfaces.end(); ++it)
    surfaces.push_back(*(it->second));
  return surfaces;
}

CSGSurface &
CSGSurfaceList::addSurface(std::unique_ptr<CSGSurface> surf)
{
  auto [it, inserted] = _surfaces.emplace(surf->getName(), std::move(surf));
  if (!inserted)
    mooseError("Surface with name " + surf->getName() + " already exists in geometry.");
  return *it->second;
}

void
CSGSurfaceList::renameSurface(const CSGSurface & surface, const std::string & name)
{
  // check that name is not already being used in _surfaces
  if (_surfaces.find(name) != _surfaces.end())
    mooseError("Surface with name " + name + " already exists in geometry.");

  // check that this surface passed in is actually in the same surface that is in the surface
  // list
  const auto & prev_name = surface.getName();
  auto nh = _surfaces.extract(prev_name);
  if (!nh || nh.mapped().get() != &surface)
    mooseError("Surface " + prev_name + " cannot be renamed to " + name +
               " as it does not exist in this CSGBase instance.");

  nh.key() = name;
  nh.mapped().get()->setName(name);
  _surfaces.insert(std::move(nh));
}

} // namespace CSG
