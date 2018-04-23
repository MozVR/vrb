/* -*- Mode: C++; tab-width: 20; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef VRB_RENDER_STATE_DOT_H
#define VRB_RENDER_STATE_DOT_H

#include "vrb/Forward.h"
#include "vrb/MacroUtils.h"
#include "vrb/ResourceGL.h"

#include "vrb/gl.h"

namespace vrb{

class RenderState : protected ResourceGL {
public:
  static RenderStatePtr Create(ContextWeak& aContext);
  GLuint Program() const;
  GLint AttributePosition() const;
  GLint AttributeNormal() const;
  GLint AttributeUV() const;
  uint32_t GetLightId() const;
  void ResetLights(const uint32_t aId);
  void AddLight(const Vector& aDirection, const Color& aAmbient, const Color& aDiffuse, const Color& aSpecular);
  void SetMaterial(const Color& aAmbient, const Color& aDiffuse, const Color& aSpecular, const float aSpecularExponent);
  void GetMaterial(Color& aAmbient, Color& aDiffuse, Color& aSpecular, float& aSpecularExponent) const;
  void SetTexture(const TexturePtr& aTexture);
  bool HasTexture() const;
  bool Enable(const Matrix& aPerspective, const Matrix& aView, const Matrix& aModel);
  void Disable();
protected:
  struct State;
  RenderState(State& aState, ContextWeak& aContext);
  ~RenderState();

  // ResourceGL interface
  void InitializeGL(Context& aContext) override;
  void ShutdownGL(Context& aContext) override;

  // RenderState interface
  GLuint LoadShader(GLenum type, const char* src);

private:
  State& m;
  RenderState() = delete;
  VRB_NO_DEFAULTS(RenderState)
};

} // namespace vrb

#endif //VRB_RENDER_STATE_DOT_H
