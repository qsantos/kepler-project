# Show, Don't Tell

![Exploring the Earth, the Moon and Jupiter](media/explore.ogv)

![Observing how the phases of the Moon work](media/moon.ogv)

![Piloting a spaceship in orbit](media/spaceship.ogv)

# Requirements

## Ubuntu or Debian

Run the following command in your terminal:

```
$ sudo apt-get install make g++ libglew-dev libglfw3-dev libglm-dev libstb-dev libcjson-dev libassimp-dev
```

Then, from the source directory, type:

```
$ make
$ ./test
$ ./gui
```

## Windows

### Cross-compiling from Debian

Run:

```
$ sudo ./cross-compilation-deps
```

### MSYS2

**NOTE:** there is currently a bug with GLFW 3.3 that prevents you from
linking. See [related bug](https://github.com/glfw/glfw/issues/1547).

You will need to manually install the following libraries:

- [cJSON](https://github.com/DaveGamble/cJSON/)
- [GLM](https://github.com/g-truc/glm/)
- [GLEW](https://github.com/nigels-com/glew/)
- [GLFW](https://github.com/glfw/glfw/)
- [stb_image.h](https://github.com/nothings/stb/blob/master/stb_image.h)
- [assimp](https://github.com/assimp/assimp/)
