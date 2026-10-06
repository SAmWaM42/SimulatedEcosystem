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
        // depth checking
        if (!window->frame_buffer[y][x].valid)
        {
            window->frame_buffer[y][x] = {z, {0, 0, 0}};
        }
        else
        {
            if (z < window->frame_buffer[y][x].z)
            {
                window->frame_buffer[y][x] = {z, {0, 0, 0}};
            }
        }
        p = (2 * dy) - dx;
        for (int i = 0; i <= dx; i++)
        {

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
                window->frame_buffer[y][x] = {z, {0, 0, 0},true};
            }
            else
            {
                if (z < window->frame_buffer[y][x].z)
                {
                    window->frame_buffer[y][x] = {z, {0, 0, 0},true};
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
void line::set_edge_points(Viewport *window)
{
   
    Vector2 screen_origin = window->boundary;
    float width = window->width;
    float height = window->height;
    // check if the endpoints are within the viewport

    float X_min = screen_origin.x;
    float X_max = screen_origin.x + width;
    float Y_min = screen_origin.y;
    float Y_max = screen_origin.y + height;
    bool valid = false;
    // converrt 3d co-ordinated to 2_d screen space
    std::vector<Vector2> projected_cords = {{(endpoints[0].x / endpoints[0].z), (endpoints[0].y / endpoints[0].z)}, {(endpoints[1].x / endpoints[1].z), (endpoints[1].y / endpoints[1].z)}};
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
            unsigned int val=codes[i];
            if (codes[i] >= (8))
            {
                // top
                projected_cords[i].y = Y_max;
                val &= ~8;;
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
    if (valid)
    {
        // enpoints within clipping have been found now find appropriate pixels
        // line drawing
        get_candidate_pixels(projected_cords[0].x, projected_cords[1].x, projected_cords[0].y, projected_cords[1].y, endpoints[0].z, window);
    }
}
void shape::set_unfiiled_shape(std::vector<Vector3> endpoints, Viewport *window)
{

    for (int i = 0; i < endpoints.size(); i++)
    {
        int index = (i + 1) % endpoints.size();

        Vector3 a = endpoints[i];
        Vector3 b = endpoints[index];
        line temp;
        temp.set_endpoints(a, b);
        temp.set_edge_points(window);
        edges.push_back(temp);
    }
}
void shape::set_fiiled_shape(std::vector<Vector3> endpoints, Vector3 colour, Viewport *window)
{
    for (int i = 0; i < endpoints.size(); i++)
    {
        int index = (i + 1) % endpoints.size();
        Vector3 a = endpoints[i];
        Vector3 b = endpoints[index];
        line temp;
        temp.set_endpoints(a, b);
        temp.set_edge_points(window);
        edges.push_back(temp);
    }
    // implement scanline filling
    // get min and max y for fill
    auto [minIt, maxIt] = std::minmax_element(
        endpoints.begin(),
        endpoints.end(),
        [](const Vector3 &a, const Vector3 &b)
        {
            return a.y < b.y;
        });
    std::map<int, std::vector<int>> to_fill;
    int y_min = minIt->y;
    int y_max = maxIt->y;
    int z;
    for (int i = y_min; i <= y_max; i++)
    {
        for (int j = 0; j < edges.size(); j++)
        {
            float y_max = 0;
            float y_min = 0;
            if (edges[j].endpoints[0].y - edges[j].endpoints[1].y <= 0)
            {
                y_max = edges[j].endpoints[1].y;
                y_min = edges[j].endpoints[0].y;
            }
            else
            {
                y_max = edges[j].endpoints[0].y;
                y_min = edges[j].endpoints[1].y;
            }

            if (i > y_min && i < y_max)
            {
                float x = edges[j].endpoints[0].x + (i - edges[j].endpoints[1].y) * (edges[j].endpoints[1].x - edges[j].endpoints[0].x) / (edges[j].endpoints[1].y - edges[j].endpoints[0].y);
                to_fill[i].push_back(x);
                z = edges[j].endpoints[0].z;
            }
        }
    }
    // continue here with a sort and fill
}
void shape::print_framebuffer(Viewport* window){

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