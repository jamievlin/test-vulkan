//
// Created by Supakorn on 11/12/2021.
//

#pragma once
#include "common.h"
#include "Mesh.h"
#include "UniformObjects.h"

class Drawable : public AVkGraphicsBase
{
private:
    Mesh* drawMesh;
public:
    Drawable(Mesh* mesh) : drawMesh(mesh), uniform(glm::mat4())
    {
    }

    Drawable(Mesh* mesh, glm::mat4 model) : drawMesh(mesh), uniform(model)
    {
    }
    ~Drawable() = default;

    Drawable(Drawable&& dwb) noexcept :
        AVkGraphicsBase(std::move(dwb)),
        drawMesh(dwb.drawMesh),
        uniform(std::move(dwb.uniform))
    {
    }

    Drawable& operator= (Drawable&& dwb) noexcept
    {
        AVkGraphicsBase::operator=(std::move(dwb));
        drawMesh = std::move(dwb.drawMesh);
        uniform = std::move(uniform);

        return *this;
    }

    MeshUniform uniform;

    Mesh& getMesh()
    {
        return *drawMesh;
    }
};