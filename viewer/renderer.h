#ifndef __RENDERER_H__
#define __RENDERER_H__

#include "dataloader.h"
#include "viewport.h"
#include "utils.h"

struct RenderConfig{
    
    enum RenderMode {
        COLOR_SH_0,
        COLOR_SH_1,
        COLOR_SH_2,
        COLOR_SH_3,
        DEPTH,
        GAUSS_BALL,
        SURFEL,
    };
    
    // system setting
    RenderMode  render_mode     = COLOR_SH_3;
    bool        vsync           = true;
    bool        depth_sort      = true;

    // camera setting
    float       scale_modifier  = 1.0f;
    float       fov             = 60.0f;

    // data info
    size_t      num_primitives  = 0;
    size_t      max_sh_dim      = 4 * 4 * 3;
};

class Renderer {

public:

    Renderer(const GaussianData& data, Shader* shader): _data(data) , _shader(shader){
        
        std::vector<float> _vertices = {-1.0f,  1.0f, 1.0f,  1.0f, 1.0f, -1.0f, -1.0f, -1.0f};

        _config = RenderConfig();
        _config.num_primitives = _data.size();
        _config.max_sh_dim = _data.sh_dim();

        glGenVertexArrays(1, &_vao);
        glGenBuffers(1, &_vbo);
        glBindVertexArray(_vao);
        glBindBuffer(GL_ARRAY_BUFFER, _vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 8, _vertices.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
        glEnableVertexAttribArray(0);

        std::vector<float> flatData = _data.flat();
        glGenBuffers(1, &_ssbo_splat);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, _ssbo_splat);
        glBufferData(GL_SHADER_STORAGE_BUFFER, flatData.size() * sizeof(float), flatData.data(), GL_STATIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, _ssbo_splat);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        _index = sort(_data, Eigen::Matrix4f::Identity());

        glDisable(GL_CULL_FACE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    void render(const Viewport viewport){

        Eigen::Matrix4f projmat = viewport.getProjectionMatrix();
        Eigen::Matrix4f viewmat = viewport.getViewMatrix();
        Eigen::Vector3f cam_pos = viewport.camera.getPosition();
        Eigen::Vector2f tanxy = viewport.getTanXY();
        float focal = viewport.getFocal();

        _shader->set_uniform("projmat", projmat);
        _shader->set_uniform("viewmat", viewmat);
        _shader->set_uniform("cam_pos", cam_pos);
        _shader->set_uniform("focal", focal);
        _shader->set_uniform("tanxy", tanxy);
        _shader->set_uniform("max_sh_dim", _config.max_sh_dim);
        _shader->set_uniform("render_mod", _config.render_mode);
        _shader->set_uniform("scale_modifier", _config.scale_modifier);

        if (_config.depth_sort) {
            _index = sort(_data, viewmat);
        }

        glGenBuffers(1, &_ssbo_index);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, _ssbo_index);
        glBufferData(GL_SHADER_STORAGE_BUFFER, _index.size() * sizeof(int), _index.data(), GL_STATIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, _ssbo_index);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        
        _shader->bind(false);
        glBindVertexArray(_vao);
        glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, static_cast<int>(_data.size()));
    }

    RenderConfig& config() {
        return _config;
    }

private:
    GLuint              _vao;
    GLuint              _vbo;
    GLuint              _ssbo_splat;
    GLuint              _ssbo_index;
    Shader*             _shader;
    RenderConfig        _config;
    const GaussianData& _data;
    std::vector<int>    _index;

    Timer               _timer;
};

#endif 