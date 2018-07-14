/* -*- Mode: C++; tab-width: 20; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "vrb/Geometry.h"

#include "vrb/private/DrawableState.h"
#include "vrb/private/NodeState.h"
#include "vrb/private/ResourceGLState.h"

#include "vrb/Camera.h"
#include "vrb/ConcreteClass.h"
#include "vrb/CullVisitor.h"
#include "vrb/DrawableList.h"
#include "vrb/GLError.h"
#include "vrb/Logger.h"
#include "vrb/Matrix.h"
#include "vrb/RenderState.h"
#include "vrb/Texture.h"
#include "vrb/VertexArray.h"
#include "vrb/Vector.h"

#include <vector>

namespace {

static void
CopyIndecies(std::vector<GLushort> &aTarget, const std::vector<int> &aSource) {
  aTarget.reserve(aSource.size());
  for (auto value: aSource) {
    if (value >= std::numeric_limits<GLushort>::max()) {
      VRB_ERROR("Index is greater than max size of GLushort: %d", value);
    }
    aTarget.push_back(static_cast<GLushort>(value));
  }
}

}

namespace vrb {

struct Geometry::State : public Node::State, public ResourceGL::State, public Drawable::State {
  RenderStatePtr renderState;
  VertexArrayPtr vertexArray;
  std::vector<Face> faces;
  int vertexCount;
  int triangleCount;
  GLuint vertexObjectId;
  GLuint indexObjectId;

  State() : vertexCount(0), triangleCount(0), vertexObjectId(0), indexObjectId(0) {}


  GLsizei UVLength() const {
    vrb::TexturePtr texture = renderState->GetTexture();
    if (!texture) {
      return 0;
    }
    return texture && texture->GetTarget() == GL_TEXTURE_CUBE_MAP ? 3 : 2;
  }

  GLsizei UVSize() const {
    return UVLength() * sizeof(float);
  }

  GLsizei PositionSize() const {
    return 3 * sizeof(float);
  }

  GLsizei NormalSize() const {
    return 3 * sizeof(float);
  }

  GLsizei VertexSize() const {
    return PositionSize() + NormalSize() + UVSize();
  }

};

GeometryPtr
Geometry::Create(CreationContextPtr& aContext) {
  return std::make_shared<ConcreteClass<Geometry, Geometry::State> >(aContext);
}

// Node interface
void
Geometry::Cull(CullVisitor& aVisitor, DrawableList& aDrawables) {
  aDrawables.AddDrawable(std::move(CreateDrawablePtr()), aVisitor.GetTransform());
}

// Drawable interface
RenderStatePtr&
Geometry::GetRenderState() {
  return m.renderState;
}

void
Geometry::SetRenderState(const RenderStatePtr& aRenderState) {
  m.renderState = aRenderState;
}

void
Geometry::Draw(const Camera& aCamera, const Matrix& aModelTransform) {
  if (m.renderState->Enable(aCamera.GetPerspective(), aCamera.GetView(), aModelTransform)) {
    const bool kUseTextureCoords = m.renderState->HasTexture();
    const GLsizei kSize = m.VertexSize();
    const GLsizei kPositionSize = m.PositionSize();
    const GLsizei kNormalSize = m.NormalSize();
    const GLsizei kUVLength = m.UVLength();
    VRB_GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, m.vertexObjectId));
    VRB_GL_CHECK(glVertexAttribPointer((GLuint)m.renderState->AttributePosition(), 3, GL_FLOAT, GL_FALSE, kSize, nullptr));
    VRB_GL_CHECK(glVertexAttribPointer((GLuint)m.renderState->AttributeNormal(), 3, GL_FLOAT, GL_FALSE, kSize, (const GLvoid*)kPositionSize));
    if (kUseTextureCoords) {
      VRB_GL_CHECK(glVertexAttribPointer((GLuint)m.renderState->AttributeUV(), kUVLength, GL_FLOAT, GL_FALSE, kSize, (const GLvoid*)(kPositionSize + kNormalSize)));
    }

    VRB_GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.indexObjectId));
    VRB_GL_CHECK(glEnableVertexAttribArray((GLuint)m.renderState->AttributePosition()));
    VRB_GL_CHECK(glEnableVertexAttribArray((GLuint)m.renderState->AttributeNormal()));
    if (kUseTextureCoords) {
      VRB_GL_CHECK(glEnableVertexAttribArray((GLuint)m.renderState->AttributeUV()));
    }
    VRB_GL_CHECK(glDrawElements(GL_TRIANGLES, m.triangleCount * 3, GL_UNSIGNED_SHORT, 0));
    VRB_GL_CHECK(glDisableVertexAttribArray((GLuint)m.renderState->AttributePosition()));
    VRB_GL_CHECK(glDisableVertexAttribArray((GLuint)m.renderState->AttributeNormal()));
    if (kUseTextureCoords) {
      VRB_GL_CHECK(glDisableVertexAttribArray((GLuint)m.renderState->AttributeUV()));
    }
    m.renderState->Disable();
    VRB_GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
    VRB_GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, 0));
  }
}

// Geometry interface
VertexArrayPtr
Geometry::GetVertexArray() const {
  return m.vertexArray;
}

void
Geometry::SetVertexArray(const VertexArrayPtr& aVertexArray) {
  m.vertexArray = aVertexArray;
}

void
Geometry::UpdateBuffers() {
  if (m.vertexObjectId == 0 || m.indexObjectId == 0) {
    VRB_WARN("Geometry GL objects not created");
    return;
  }
  const bool kUseTextureCoords = m.renderState->HasTexture();
  VRB_GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, m.vertexObjectId));
  const GLintptr kVectorSize = m.PositionSize();
  GLintptr kUVSize = m.UVSize();

  std::vector<GLushort> indices;
  GLushort count = 0;
  GLintptr offset = 0;

  for (auto& face: m.faces) {
    if (face.vertices.size() == 0) {
      break;
    }
    if (face.vertices.size() < 3) {
      std::string message;
      for (auto index: face.vertices) { message += " "; message += std::to_string(index); }
      VRB_ERROR("Face with only %d vertices:%s", (int32_t)face.vertices.size(), message.c_str());
      continue;
    }
    const GLushort vertexIndex = (GLushort)(face.vertices[0] - 1);
    const GLushort normalIndex = (GLushort)(face.normals[0] - 1);
    const GLushort uvIndex = (GLushort)(kUseTextureCoords ? face.uvs[0] - 1 : -1);
    const Vector& firstVertex = m.vertexArray->GetVertex(vertexIndex);
    const Vector& firstNormal = m.vertexArray->GetNormal(normalIndex);
    const Vector& firstUV = m.vertexArray->GetUV(uvIndex);
    for (int ix = 1; ix <= face.vertices.size() - 2; ix++) {
      std::string out;
      VRB_GL_CHECK(glBufferSubData(GL_ARRAY_BUFFER, offset, kVectorSize, firstVertex.Data()));
      offset += kVectorSize;
      VRB_GL_CHECK(glBufferSubData(GL_ARRAY_BUFFER, offset, kVectorSize, firstNormal.Data()));
      offset += kVectorSize;
      out += " " + firstVertex.ToString() + firstNormal.ToString();
      if (kUseTextureCoords) {
        VRB_GL_CHECK(glBufferSubData(GL_ARRAY_BUFFER, offset, kUVSize, firstUV.Data()));
        offset += kUVSize;
        out += firstUV.ToString();
      }
      indices.push_back(count);
      count++;

      const Vector v1 = m.vertexArray->GetVertex(face.vertices[ix] - 1);
      const Vector n1 = m.vertexArray->GetNormal(face.normals[ix] - 1);
      VRB_GL_CHECK(glBufferSubData(GL_ARRAY_BUFFER, offset, kVectorSize, v1.Data()));
      offset += kVectorSize;
      VRB_GL_CHECK(glBufferSubData(GL_ARRAY_BUFFER, offset, kVectorSize, n1.Data()));
      offset += kVectorSize;
      out += "/" + v1.ToString() + n1.ToString();
      if (kUseTextureCoords) {
        const Vector uv1 = m.vertexArray->GetUV(face.uvs[ix] - 1);
        VRB_GL_CHECK(glBufferSubData(GL_ARRAY_BUFFER, offset, kUVSize, uv1.Data()));
        offset += kUVSize;
        out += uv1.ToString();
      }
      indices.push_back(count);
      count++;

      const Vector v2 = m.vertexArray->GetVertex(face.vertices[ix + 1] - 1);
      const Vector n2 = m.vertexArray->GetNormal(face.normals[ix + 1] - 1);
      VRB_GL_CHECK(glBufferSubData(GL_ARRAY_BUFFER, offset, kVectorSize, v2.Data()));
      offset += kVectorSize;
      VRB_GL_CHECK(glBufferSubData(GL_ARRAY_BUFFER, offset, kVectorSize, n2.Data()));
      offset += kVectorSize;
      out += "/" + v2.ToString() + n2.ToString();
      if (kUseTextureCoords) {
        const Vector uv2 = m.vertexArray->GetUV(face.uvs[ix + 1] - 1);
        VRB_GL_CHECK(glBufferSubData(GL_ARRAY_BUFFER, offset, kUVSize, uv2.Data()));
        offset += kUVSize;
        out += uv2.ToString();
      }
      indices.push_back(count);
      count++;
//      VRB_LOG("array buffer[%d] o:%d c:%d : %s", ix, (int)offset, count, out.c_str());
    }
  }

  VRB_GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.indexObjectId));
  VRB_GL_CHECK(glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(GLushort) * indices.size(), indices.data()));


  VRB_GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
  VRB_GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, 0));
}

void
Geometry::AddFace(
    const std::vector<int>& aVertices,
    const std::vector<int>& aUVs,
    const std::vector<int>& aNormals) {

  Face face;
  m.vertexCount += aVertices.size();
  m.triangleCount += aVertices.size() - 2;
  CopyIndecies(face.vertices, aVertices);
  if (face.vertices.size() < 3) {
    std::string indices;
    for (auto ix: face.vertices) {
      indices += " ";
      indices += std::to_string(ix);
    }
    indices += " from:";
    for (auto ix: aVertices) {
      indices += " ";
      indices += std::to_string(ix);
    }
    VRB_ERROR("Face with only %d vertices:%s", (int)face.vertices.size(), indices.c_str());
  }
  if (aUVs.size() > 0) {
    CopyIndecies(face.uvs, aUVs);
  }

  if (aNormals.size() > 0 && (aNormals[0] != 0)) {
    CopyIndecies(face.normals, aNormals);
  } else if (m.vertexArray) {
    m.vertexArray->SetNormalCount(m.vertexArray->GetVertexCount());
    const Vector point = m.vertexArray->GetVertex(aVertices[0] - 1);
    const Vector normal = ((m.vertexArray->GetVertex(aVertices[1] - 1) - point).Cross(m.vertexArray->GetVertex(aVertices[2] - 1) - point)).Normalize();
    int place = m.vertexArray->AppendNormal(normal);
    for (GLuint index: face.vertices) {
      if (index <= 0) {
        VRB_ERROR("Vertices index is less than zero.");
      }
      if (normal.Magnitude() > 0.00001f) {
        m.vertexArray->AddNormal(index - 1, normal);
      }
      face.normals.push_back(index);
    }
  }

  m.faces.push_back(std::move(face));
}

int32_t
Geometry::GetFaceCount() const {
  return m.faces.size();
}

const Geometry::Face&
Geometry::GetFace(int32_t aIndex) const {
  return m.faces[aIndex];
}

Geometry::Geometry(State& aState, CreationContextPtr& aContext) :
    Node(aState, aContext),
    ResourceGL(aState, aContext),
    Drawable(aState, aContext),
    m(aState)
{}
Geometry::~Geometry() {}

// ResourceGL interface
bool
Geometry::SupportOffRenderThreadInitialization() {
  return true;
}

void
Geometry::InitializeGL() {
  if (!m.renderState) {
    VRB_ERROR("Unable to initialize Geometry Node. No RenderState set");
  }
  VRB_GL_CHECK(glGenBuffers(1, &m.vertexObjectId));
  VRB_GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, m.vertexObjectId));
  GLsizei kTriangleSize = m.VertexSize() * 3;

  VRB_GL_CHECK(glBufferData(GL_ARRAY_BUFFER, kTriangleSize * m.triangleCount, nullptr, GL_STATIC_DRAW));
  VRB_LOG("Allocate: %d for GL_ARRAY_BUFFER: %d", kTriangleSize * m.triangleCount, m.vertexObjectId);

  VRB_GL_CHECK(glGenBuffers(1, &m.indexObjectId));
  VRB_GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.indexObjectId));
  VRB_GL_CHECK(glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort) * m.triangleCount * 3, nullptr, GL_STATIC_DRAW));
  VRB_LOG("Allocate: %d for GL_ELEMENT_ARRAY_BUFFER: %d", (int32_t)sizeof(GLushort) * m.triangleCount * 3, m.indexObjectId);

  UpdateBuffers();
}

void
Geometry::ShutdownGL() {

}

}

