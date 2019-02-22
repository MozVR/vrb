/* -*- Mode: C++; tab-width: 20; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "vrb/VertexArray.h"

#include "vrb/ConcreteClass.h"
#include "vrb/Color.h"
#include "vrb/Vector.h"

#include <vector>

namespace vrb {

static const vrb::Vector cZeroVector;
static const vrb::Color cZeroColor;

struct VertexArray::State {
  struct NormalState {
    Vector normal;
    float count;
    NormalState() : count(0.0f) {}
    NormalState(const Vector& aNormal) : normal(aNormal), count(1.0f) {}
  };
  std::vector<Vector> vertices;
  std::vector<NormalState> normals;
  std::vector<Vector> uvs;
  std::vector<Color> colors;
};

VertexArrayPtr
VertexArray::Create(CreationContextPtr& aContext) {
  return std::make_shared<ConcreteClass<VertexArray, VertexArray::State> >(aContext);
}

int
VertexArray::GetVertexCount() const {
  return m.vertices.size();
}

int
VertexArray::GetNormalCount() const {
  return m.normals.size();
}

int
VertexArray::GetUVCount() const {
  return m.uvs.size();
}

int
VertexArray::GetColorCount() const {
  return m.colors.size();
}

void
VertexArray::SetNormalCount(const int aCount) {
  if (m.normals.size() < aCount) {
    m.normals.resize(aCount);
  }
}

const Vector&
VertexArray::GetVertex(const int aIndex) const {
  if (aIndex >= m.vertices.size()) {
    return cZeroVector;
  }
  return m.vertices[aIndex];
}

const Vector&
VertexArray::GetNormal(const int aIndex) const {
  if (aIndex >= m.normals.size()) {
    return cZeroVector;
  }
  return m.normals[aIndex].normal;
}

const Vector&
VertexArray::GetUV(const int aIndex) const {
  if (aIndex >= m.uvs.size()) {
    return cZeroVector;
  }
  return m.uvs[aIndex];
}


const Color&
VertexArray::GetColor(const int aIndex) const {
  if (aIndex >= m.colors.size()) {
    return cZeroColor;
  }
  return m.colors[aIndex];
}

void
VertexArray::SetVertex(const int aIndex, const Vector& aPoint) {
  if (m.vertices.size() < (aIndex + 1)) {
    m.vertices.resize(aIndex + 1);
  }
  m.vertices[aIndex] = aPoint;
}

void
VertexArray::SetNormal(const int aIndex, const Vector& aNormal) {
  if (m.normals.size() < (aIndex + 1)) {
    m.normals.resize(aIndex + 1);
  }
  m.normals[aIndex] = aNormal;
}

void
VertexArray::SetUV(const int aIndex, const Vector& aUV) {
  if (m.uvs.size() < (aIndex + 1)) {
    m.uvs.resize(aIndex + 1);
  }
  m.uvs[aIndex] = aUV;
}

void
VertexArray::SetColor(const int aIndex, const Color& aColor) {
  if (m.colors.size() < (aIndex + 1)) {
    m.colors.resize(aIndex + 1);
  }
  m.colors[aIndex] = aColor;
}

int
VertexArray::AppendVertex(const Vector& aPoint) {
  m.vertices.push_back(aPoint);
  return m.vertices.size() - 1;
}

int
VertexArray::AppendNormal(const Vector& aNormal) {
  m.normals.push_back(State::NormalState(aNormal));
  return m.normals.size() - 1;
}

void
VertexArray::AddNormal(const int aIndex, const Vector& aNormal) {
  if (m.normals.size() < (aIndex + 1)) {
    m.normals.resize(aIndex + 1);
  }
  State::NormalState& ns = m.normals[aIndex];
  const float originalCount = ns.count;
  ns.count++;
  Vector orig = ns.normal;
  ns.normal = (((ns.normal * originalCount) + aNormal) / ns.count).Normalize();
}

int
VertexArray::AppendUV(const Vector& aUV) {
  m.uvs.push_back(aUV);
  return m.uvs.size() - 1;
}

int
VertexArray::AppendColor(const Color& aColor) {
  m.colors.push_back(aColor);
  return m.colors.size() - 1;
}

VertexArray::VertexArray(State& aState, CreationContextPtr& aContext) : m(aState) {}
VertexArray::~VertexArray() {}

}

