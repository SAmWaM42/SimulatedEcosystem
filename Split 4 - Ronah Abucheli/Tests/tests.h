#pragma once
#include <primitives.h>
void test_matrix_operation()
{
    std::cout << "\n=== Matrix Operation Tests ===\n";
    // Test 1: Identity matrix
    Matrix identity;
    Vec4 point = {2.0f, 3.0f, 4.0f, 1.0f};

    Vec4 result = identity.muliplyByVec(point);

    std::cout << "\nTest 1: Identity Matrix\n";
    std::cout << "Expected: (2, 3, 4, 1)\n";
    std::cout << "Actual:   (" << result.x << ", "
              << result.y << ", " << result.z << ", "
              << result.w << ")\n";

    // Test 2: Translation matrix
    Transformer transformer;
    transformer.translate({5.0f, 0.0f, 0.0f});
    transformer.set_tranformation_matrix();

    Vec3 position = transformer.transform({2.0f, 3.0f, 4.0f}, 1.0f);
    Vec3 direction = transformer.transform({2.0f, 3.0f, 4.0f}, 0.0f);

    std::cout << "\nTest 2: Translation and Homogeneous W\n";
    std::cout << "Position expected: (7, 3, 4)\n";
    std::cout << "Position actual:   (" << position.x << ", "
              << position.y << ", " << position.z << ")\n";

    std::cout << "Direction expected: (2, 3, 4)\n";
    std::cout << "Direction actual:   (" << direction.x << ", "
              << direction.y << ", " << direction.z << ")\n";

    // Test 3: Matrix multiplication order
    Transformer combined;
    combined.rotate({0.0f, 0.0f, 90.0f});
    combined.scale({2.0f, 2.0f, 2.0f});
    combined.translate({10.0f, 0.0f, 0.0f});
    combined.set_tranformation_matrix();

    Vec3 transformed = combined.transform({1.0f, 0.0f, 0.0f}, 1.0f);

    std::cout << "\nTest 3: Combined Transformation (R -> S -> T)\n";
    std::cout << "Expected: (10, 2, 0)\n";
    std::cout << "Actual:   (" << transformed.x << ", "
              << transformed.y << ", " << transformed.z << ")\n";

    std::cout << "\n=== Tests Complete ===\n";
}



void test_shape_transformation()
{
    std::cout << "\n=== Shape Transformation Test ===\n";

    // 1. Create a shape with three vertices (a triangle).
    Shape shape;

    shape.vertices = {
        {1.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 1.0f}
    };

    // 2. Define its transformation.
    shape.transform.position = {10.0f, 0.0f, 0.0f};
    shape.transform.rotation = {0.0f, 0.0f, 90.0f};
    shape.transform.scale = {2.0f, 2.0f, 2.0f};

    // 3. Apply the transformation.
    Transformer transformer;
    shape.world_transform(transformer);

    // 4. Display the original and transformed vertices.
    std::cout << "\nLocal vertices:\n";

    for (const auto& v : shape.vertices)
    {
        std::cout << "(" << v.x << ", "
                  << v.y << ", " << v.z << ")\n";
    }

    std::cout << "\nWorld vertices:\n";

    for (const auto& v : shape.w_vertices)
    {
        std::cout << "(" << v.x << ", "
                  << v.y << ", " << v.z << ")\n";
    }

    std::cout << "\n=== Test Complete ===\n";
}
void test_camera()
{
    Camera camera;

    camera.position = {5.0f, 0.0f, 0.0f};
    camera.rotation = {0.0f, 0.0f, 0.0f};

    camera.update_view_matrix();

    Vec3 result = camera.world_to_camera({10.0f, 0.0f, 0.0f});

    std::cout << "Camera-space vertex: ("
              << result.x << ", "
              << result.y << ", "
              << result.z << ")\n";
}
void run_test()
{
    test_matrix_operation();
    test_shape_transformation();
    test_camera();
}
