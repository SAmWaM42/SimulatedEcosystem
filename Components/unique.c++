#include <unique.h>
std::vector<unsigned int> compute_code(std::vector<Vector2> endpoints, float X_min, float X_max, float Y_min, float Y_max)
{
    unsigned int inside = 0;
    unsigned int left = 1;
    unsigned int right = 2;
    unsigned int bottom = 4;
    unsigned int top = 8;

    unsigned int value = inside;
    std::vector<unsigned int> codes;
    for (auto &a : endpoints)
    {
        if (a.x < X_min)
        {
            value |= left;
        }
        else if (a.x > X_max)
        {
            value |= right;
        }
        if (a.y < Y_min)
        {
            value |= bottom;
        }
        else if (a.y > Y_max)
        {
            value |= top;
        }
        codes.push_back(value);
        value = inside;
    }

    return codes;
}
void get_candidate_pixels(int x0, int x1, int y0, int y1, int z0, Viewport *window)
{
    // has possible in-accurate depth issue for the value of z1 may end up being larger
    int p;
    int dy = abs(y1 - y0);
    int dx = abs(x1 - x0);
    int sx = 0;
    int sy = 0;
    if (x1 > x0)
    {
        sx = 1;
    }
    else
    {
        sx = -1;
    };
    if (y1 > y0)
    {
        sy = 1;
    }
    else
    {
        sy = -1;
    };
    int x = x0, y = y0, z = z0;
    if (dx >= dy)
    {
        p = (2 * dy) - dx;
        for (int i = 0; i <= dx; i++)
        {
            // depth checking
            if (!window->frame_buffer[y][x].valid)
            {
                window->frame_buffer[y][x] = {z, {1, 0, 0}, true};
            }
            else
            {
                if (z < window->frame_buffer[y][x].z)
                {
                    window->frame_buffer[y][x] = {z, {1, 0, 0}, true};
                }
            }

            if (p >= 0)
            {
                y += sy;
                p -= 2 * dx;
            }
            x += sx;
            p += 2 * dy;
        }
    }
    else
    {
        p = (2 * dx) - dy;
        for (int i = 0; i <= dy; i++)
        {
            // depth checking
            if (!window->frame_buffer[y][x].valid)
            {
                window->frame_buffer[y][x] = {z, {1, 0, 0}, true};
            }
            else
            {
                if (z < window->frame_buffer[y][x].z)
                {
                    window->frame_buffer[y][x] = {z, {1, 0, 0}, true};
                }
            }

            if (p >= 0)
            {
                x += sx;
                p -= 2 * dy;
            }
            y += sy;
            p += 2 * dx;
        }
    }
    return;
}
void line::set_endpoints(Vector3 a, Vector3 b)
{
    endpoints.push_back(a);
    endpoints.push_back(b);
}
void line::set_edge_points(Viewport *window, Transformer trans, std::vector<Vector2> *screen)
{

    // check if the endpoints are within the viewport
    float X_min = 0.0f;
    float X_max = window->width - 1.0f;
    float Y_min = 0.0f;
    float Y_max = window->height - 1.0f;
    bool valid = false;
    // converrt 3d co-ordinated to 2_d
    window->cam.view_transform(trans, &endpoints);

    std::vector<Vector2> projected_cords = {{(endpoints[0].x / endpoints[0].z), (endpoints[0].y / endpoints[0].z)}, {(endpoints[1].x / endpoints[1].z), (endpoints[1].y / endpoints[1].z)}};
    // convert 2d to screen  space
    for (auto &val : projected_cords)
    {
        val.x = ((val.x + 1) / 2) * (X_max);
        val.y = ((1 - val.y) / 2) * (Y_max);
    }

    std::vector<unsigned int> codes = compute_code(projected_cords, X_min, X_max, Y_min, Y_max);
    if ((codes[0] | codes[1]) == 0)
    {
        valid = true;
    }
    else if ((codes[0] & codes[1]) != 0)
    {
        std::cout << "line segment not in frame";
        return;
    }
    else
    {
        // line clipping to identify new endpoints for line-drawing
        for (size_t i = 0; i < codes.size(); i++)
        {
            if (codes[i] == 0)
            {
                continue;
            }
            // y-axis check
            unsigned int val = codes[i];
            if (codes[i] >= (8))
            {
                // top
                projected_cords[i].y = Y_max;
                val &= ~8;
                ;
            }
            else if (codes[i] < 8)
            { // bottom
                projected_cords[i].y = Y_min;
                val &= ~4;
            }
            // x-axis check

            if (val == 1)
            {
                // left
                projected_cords[i].x = X_min;
            }
            else if (val == 2)
            { // right
                projected_cords[i].x = X_max;
            }
        }
        valid = true;
    }
    screen->push_back(projected_cords[0]);
    screen->push_back(projected_cords[1]);
    if (valid)
    {
        // enpoints within clipping have been found now find appropriate pixels
        // line drawing
        get_candidate_pixels(projected_cords[0].x, projected_cords[1].x, projected_cords[0].y, projected_cords[1].y, endpoints[0].z, window);
    }
};

shape::shape(std::vector<Vector3> points, Transform transform, Transformer trans)
{
    this->relative_endpoints = points;
    this->transform = transform;
    for (auto val : relative_endpoints)
    {
        Vector3 n_val = val;
        n_val = trans.transform(n_val, transform);
        endpoints.push_back(n_val);
    }
}
void shape::set_unfiiled_shape(Viewport *window, Transformer trans)
{

    for (int i = 0; i < endpoints.size(); i++)
    {
        int index = (i + 1) % endpoints.size();

        Vector3 a = endpoints[i];
        Vector3 b = endpoints[index];
        line temp;
        temp.set_endpoints(a, b);
        temp.set_edge_points(window, trans, &this->screen_coords);
        edges.push_back(temp);
    }
    screen_coords.clear();
    edges.clear();
}
void shape::set_fiiled_shape(Vector3 colour, Viewport *window, Transformer trans)
{
    for (int i = 0; i < endpoints.size(); i++)
    {
        int index = (i + 1) % endpoints.size();
        Vector3 a = endpoints[i];
        Vector3 b = endpoints[index];
        line temp;
        temp.set_endpoints(a, b);
        temp.set_edge_points(window, trans, &this->screen_coords);
        edges.push_back(temp);
    }
    // implement scanline filling
    // get min and max y for fill

    auto [minIt, maxIt] = std::minmax_element(
        this->screen_coords.begin(),
        this->screen_coords.end(),
        [](const Vector2 &a, const Vector2 &b)
        {
            return a.y < b.y;
        });

    std::map<int, std::vector<int>> to_fill;
    int Y_min = minIt->y;
    int Y_max = maxIt->y;
    int z;
    for (int i = Y_min; i <= Y_max; i++)
    {
        for (int j = 0; j < edges.size(); j++)
        {
            Vector2 a = screen_coords[j * 2];
            Vector2 b = screen_coords[j * 2 + 1];

            float y_max = 0;
            float y_min = 0;

            if (a.y <= b.y)
            {
                y_max = b.y;
                y_min = a.y;
            }
            else
            {
                y_max = a.y;
                y_min = b.y;
            }

            bool passes = (i > y_min && i < y_max);
            
            if (passes)
            {
                
                float x =
                    a.x +
                    (i - a.y) *
                        (b.x - a.x) /
                        (b.y - a.y);

                to_fill[i].push_back(x);
            }
        }

        // continue here with a sort and fill
        for (auto &[y, intersections] : to_fill)
        {
            std::sort(intersections.begin(), intersections.end());
            for (size_t i = 0; i + 1 < intersections.size(); i += 2)
            {
                int a = intersections[i];
                int b = intersections[i + 1];

                for (int j = a; j < b; j++)
                {

                    window->frame_buffer[y][j].rgb = colour;
                    window->frame_buffer[y][j].valid = true;
                    window->frame_buffer[y][j].z = z;
                }
            }
        }
        screen_coords.clear();
        edges.clear();
    }
}
void shape::print_framebuffer(Viewport *window)
{

    for (int y = 0; y < window->height; ++y)
    {
        for (int x = 0; x < window->width; ++x)
        {
            if (window->frame_buffer[y][x].valid)
            {
                std::cout << "(" << x << ", " << y << ")"
                          << " z=" << window->frame_buffer[y][x].z
                          << '\n';
            }
        }
    }
}

// transformation matrix implementation
Vector3 Transformer::rotate(Vector3 points, Vector3 rotation)
{
    Vector3 rotated_values;
    // rotate around x
    float r_x = rotation.x * M_PI / 180.0f;
    float r_y = rotation.y * M_PI / 180.0f;
    float r_z = rotation.z * M_PI / 180.0f;

    rotated_values.x =
        (cos(r_z) * cos(r_y) * points.x) + ((cos(r_z) * sin(r_y) * sin(r_x)) - (sin(r_z) * cos(r_x))) * points.y + ((cos(r_z) * sin(r_y) * cos(r_x)) + (sin(r_z) * sin(r_x))) * points.z;

    rotated_values.y =
        (sin(r_z) * cos(r_y) * points.x) + ((sin(r_z) * sin(r_y) * sin(r_x)) + (cos(r_z) * cos(r_x))) * points.y + ((sin(r_z) * sin(r_y) * cos(r_x)) - (cos(r_z) * sin(r_x))) * points.z;

    rotated_values.z =
        (-sin(r_y) * points.x) + (cos(r_y) * sin(r_x) * points.y) + (cos(r_y) * cos(r_x) * points.z);

    return rotated_values;
};
Vector3 Transformer::rotate_inverse(Vector3 points, Vector3 rotation)
{
    Vector3 rotated_values;
    // rotate around x
    float r_x = rotation.x * M_PI / 180.0f;
    float r_y = rotation.y * M_PI / 180.0f;
    float r_z = rotation.z * M_PI / 180.0f;
    rotated_values.x =
        (cos(r_y) * cos(r_z) * points.x) + (-cos(r_y) * sin(r_z) * points.y) + (sin(r_y) * points.z);

    rotated_values.y =
        ((cos(r_x) * sin(r_z)) +
         (sin(r_x) * sin(r_y) * cos(r_z))) *
            points.x +
        ((cos(r_x) * cos(r_z)) -
         (sin(r_x) * sin(r_y) * sin(r_z))) *
            points.y -
        (sin(r_x) * cos(r_y)) * points.z;

    rotated_values.z =
        ((sin(r_x) * sin(r_z)) -
         (cos(r_x) * sin(r_y) * cos(r_z))) *
            points.x +
        ((sin(r_x) * cos(r_z)) +
         (cos(r_x) * sin(r_y) * sin(r_z))) *
            points.y +
        (cos(r_x) * cos(r_y)) * points.z;

    return rotated_values;
};

Vector3 Transformer::scale(Vector3 points, Vector3 scale)
{
    Vector3 scaled = {};
    scaled.x = points.x * scale.x;
    scaled.y = points.y * scale.y;
    scaled.z = points.z * scale.z;
    return scaled;
}
Vector3 Transformer::translate(Vector3 points, Vector3 position)
{
    Vector3 translated;
    translated.x = points.x + position.x;
    translated.y = points.y + position.y;
    translated.z = points.z + position.z;

    return translated;
}
Vector3 Transformer::transform(Vector3 points, Transform transform)
{
    Vector3 to_return = points;
    to_return = this->scale(to_return, transform.scale);
    to_return = this->rotate(to_return, transform.rotation);
    to_return = this->translate(to_return, transform.position);

    return to_return;
}

// view transformation
void Camera::view_transform(Transformer trans, std::vector<Vector3> *points)
{
    Vector3 temp;
    for (auto &point : *points)
    {
        // point relative to camera
        temp.x = point.x - this->transform.position.x;
        temp.y = point.y - this->transform.position.y;
        temp.z = point.z - this->transform.position.z;

        // rorate points relative to camera rotation
        temp = trans.rotate_inverse(temp, this->transform.rotation);
        point = temp;
    }
    return;
}