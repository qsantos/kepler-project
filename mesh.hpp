struct UVSphereMesh {
    UVSphereMesh(float radius, int stacks, int slices);
    void bind(void);
    void draw(void);

    int components;
    int length;
    unsigned vbo;
};
