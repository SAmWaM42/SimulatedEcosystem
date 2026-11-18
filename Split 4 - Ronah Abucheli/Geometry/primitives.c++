#include <primitives.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
// angle transformations
float degree_to_radians(float angle) { return (angle * (M_PI / 180)); };
float radians_to_degrees(float radians) { return (radians * (180 / M_PI)); };
bool approximately_equal(float a, float b, float epsilon)
{
    return std::abs(a - b) < epsilon;
};
// matrix implementation

Matrix::Matrix()
{
    for (int i = 0; i < 4; i++)
    {
        this->m_values[i] = identity_matrix[i];
    };
} // ensures that used matrices start as 4*4 identity matrix
Matrix Matrix::inverse_rotation(const Matrix& rotation_matrix)
{
    Matrix result;

    result.m_values[0] = {
        rotation_matrix.m_values[0].x,
        rotation_matrix.m_values[1].x,
        rotation_matrix.m_values[2].x,
        0.0f
    };

    result.m_values[1] = {
        rotation_matrix.m_values[0].y,
        rotation_matrix.m_values[1].y,
        rotation_matrix.m_values[2].y,
        0.0f
    };

    result.m_values[2] = {
        rotation_matrix.m_values[0].z,
        rotation_matrix.m_values[1].z,
        rotation_matrix.m_values[2].z,
        0.0f
    };

    result.m_values[3] = {0.0f, 0.0f, 0.0f, 1.0f};

    return result;
}
//inverse
Vec4 Matrix::muliplyByVec(Vec4 val)
{
    Vec4 to_Return = {};
    to_Return.x = (m_values[0].x * val.x) + (m_values[0].y * val.y) + (m_values[0].z * val.z) + (m_values[0].w * val.w);
    to_Return.y = (m_values[1].x * val.x) + (m_values[1].y * val.y) + (m_values[1].z * val.z) + (m_values[1].w * val.w);
    to_Return.z = (m_values[2].x * val.x) + (m_values[2].y * val.y) + (m_values[2].z * val.z) + (m_values[2].w * val.w);
    to_Return.w = (m_values[3].x * val.x) + (m_values[3].y * val.y) + (m_values[3].z * val.z) + (m_values[3].w * val.w);

    return to_Return;
};
Matrix Matrix::muliplyBymat(Matrix val)
{
    // implementation
    Matrix to_Return;
    // get the columns of the resulting matrix
    Vec4 col1 = muliplyByVec({val.m_values[0].x, val.m_values[1].x, val.m_values[2].x, val.m_values[3].x});
    Vec4 col2 = muliplyByVec({val.m_values[0].y, val.m_values[1].y, val.m_values[2].y, val.m_values[3].y});
    Vec4 col3 = muliplyByVec({val.m_values[0].z, val.m_values[1].z, val.m_values[2].z, val.m_values[3].z});
    Vec4 col4 = muliplyByVec({val.m_values[0].w, val.m_values[1].w, val.m_values[2].w, val.m_values[3].w});

    // set the columns
    to_Return.m_values[0].x = col1.x;
    to_Return.m_values[1].x = col1.y;
    to_Return.m_values[2].x = col1.z;
    to_Return.m_values[3].x = col1.w;
    to_Return.m_values[0].y = col2.x;
    to_Return.m_values[1].y = col2.y;
    to_Return.m_values[2].y = col2.z;
    to_Return.m_values[3].y = col2.w;
    to_Return.m_values[0].z = col3.x;
    to_Return.m_values[1].z = col3.y;
    to_Return.m_values[2].z = col3.z;
    to_Return.m_values[3].z = col3.w;
    to_Return.m_values[0].w = col4.x;
    to_Return.m_values[1].w = col4.y;
    to_Return.m_values[2].w = col4.z;
    to_Return.m_values[3].w = col4.w;

    return to_Return;
}

// Transformer implementation
void Transformer::rotate(Vec3 rotation)
{
    float x = degree_to_radians(rotation.x);
    float y = degree_to_radians(rotation.y);
    float z = degree_to_radians(rotation.z);

    float cx = cos(x), sx = sin(x);
    float cy = cos(y), sy = sin(y);
    float cz = cos(z), sz = sin(z);

    // Rotation around X
    rotation_matrix.m_values[0] = {1, 0, 0, 0};
    rotation_matrix.m_values[1] = {0, cx, -sx, 0};
    rotation_matrix.m_values[2] = {0, sx, cx, 0};
    rotation_matrix.m_values[3] = {0, 0, 0, 1};

    Matrix rx = rotation_matrix;

    // Rotation around Y
    rotation_matrix.m_values[0] = {cy, 0, sy, 0};
    rotation_matrix.m_values[1] = {0, 1, 0, 0};
    rotation_matrix.m_values[2] = {-sy, 0, cy, 0};
    rotation_matrix.m_values[3] = {0, 0, 0, 1};

    Matrix ry = rotation_matrix;

    // Rotation around Z
    rotation_matrix.m_values[0] = {cz, -sz, 0, 0};
    rotation_matrix.m_values[1] = {sz, cz, 0, 0};
    rotation_matrix.m_values[2] = {0, 0, 1, 0};
    rotation_matrix.m_values[3] = {0, 0, 0, 1};

    Matrix rz = rotation_matrix;

    // Apply X, then Y, then Z (column-vector convention)
    rotation_matrix = rz.muliplyBymat(ry.muliplyBymat(rx));
}

void Transformer::scale(Vec3 factor)
{
    scale_matrix.m_values[0] = {factor.x, 0, 0, 0};
    scale_matrix.m_values[1] = {0, factor.y, 0, 0};
    scale_matrix.m_values[2] = {0, 0, factor.z, 0};
    scale_matrix.m_values[3] = {0, 0, 0, 1};
}

void Transformer::translate(Vec3 offset)
{
    translation_matrix.m_values[0] = {1, 0, 0, offset.x};
    translation_matrix.m_values[1] = {0, 1, 0, offset.y};
    translation_matrix.m_values[2] = {0, 0, 1, offset.z};
    translation_matrix.m_values[3] = {0, 0, 0, 1};
}
void Transformer::set_tranformation_matrix()
{
    transformation_matrix = translation_matrix.muliplyBymat(scale_matrix.muliplyBymat(rotation_matrix));
}
Vec3 Transformer::transform(Vec3 position, float w)
{
    Vec4 temp = transformation_matrix.muliplyByVec({position.x, position.y, position.z, w});
    return {temp.x, temp.y, temp.z};
}

// shape transformation

void Shape::world_transform(Transformer t_form)
{
    w_vertices.clear();
    t_form.rotate(transform.rotation);
    t_form.scale(transform.scale);
    t_form.translate(transform.position);
    t_form.set_tranformation_matrix();
    for (const auto &val : vertices)
    {
        Vec3 n_Vertice = t_form.transform(val, 1);
        w_vertices.push_back(n_Vertice);
    }
    return;
}
// camera work
void Camera::update_view_matrix()
{
    // Build the camera's forward rotation matrix.
    Transformer rotation_builder;
    rotation_builder.rotate(rotation);

    // A view matrix must undo the camera's rotation.
    Matrix inverse_rot =inverse_rot.inverse_rotation(rotation_builder.rotation_matrix);

    // Build the inverse translation.
    Transformer translation_builder;
    translation_builder.translate({
        -position.x,
        -position.y,
        -position.z
    });

    // View = inverse rotation * inverse translation.
    view_matrix = inverse_rot.muliplyBymat(
        translation_builder.translation_matrix
    );
}
Vec3 Camera::world_to_camera(Vec3 point)
{
    Vec4 result = view_matrix.muliplyByVec({
        point.x,
        point.y,
        point.z,
        1.0f
    });

    return {result.x, result.y, result.z};
}
// projection'
bool Projection::project_point(Vec3 camera_point, Vec2& projected)
{
    // Reject points outside the camera's depth range.
    if (camera_point.z <= near_plane ||
        camera_point.z > far_plane)
    {
        return false;
    }

    // Calculate the focal scale from the vertical field of view.
    float f = 1.0f / std::tan(degree_to_radians(fov) / 2.0f);

    // Perspective projection into normalized device coordinates.
    projected.x = (f / aspect_ratio) *
                  (camera_point.x / camera_point.z);

    projected.y = f *
                  (camera_point.y / camera_point.z);

    return true;
}

// scene implementation
void Scene::add_shape(std::unique_ptr<Shape> n_Shape)
{
    shapes.push_back(std::move(n_Shape));
};
void Scene::remove_shape(Shape *n_shape)
{
    for (auto it = shapes.begin(); it != shapes.end(); ++it)
    {
        if (it->get() == n_shape)
        {
            shapes.erase(it);
            return;
        }
    };
}
