# v0.6.0 - Unreleased
- Fix execution on Nvidia GPUs
- Update help message

# v0.5.1 - 2019-08-28
- Fix crash on Windows due to uninitialized data

# v0.5.0 - 2019-08-27
- Changed keybinding for wireframe mode from W to T
- Added keybindings to effect the orientation of the rocket
- Added keybindings to effect the throttle of the rocket
- Add level indicator to navball
- Add velocity markers to navball
- Add frame around navball
- Add throttle indicator
- Fix missing zeros in altitude and distance indicators
- Only show real time-warp
- More stable FPS measurement
- Add support for rendering of hyperbolic and parabolic orbits
- Turn rocket engine on and throttle is non-zero
- Disable window minimization on loss of focus
- Rocket can now change from one sphere of influence to another
- Disable numerical integration when rocket engine is off

# v0.4.0 - 2019-08-24
- Clamp camera altitude to [1 mm, 1 Tm]
- Fix help message randomly not showing up
- Fix vertical position of help message
- Added keybinding to toggle vertical synchronization
- Round displayed FPS to an integral value
- Fix orbital plane of various moons
- Fix rotation direction of Uranus
- Fix lighting of Sun, planets and moons (only Earth was okay)
- Fix selection of Earth by clicking on it
- Make rocket focusable
- Fix seams in Earth texture
- Fix orbits randomly showing up as white
- Fix random position markers showing up
- Fix rocket appearing out of orbit when zoomed on
- Make rocket rotate around its axis
- Turn rocket engine off
- Fix rendering of close objects
- Add navball

# v0.3.0 - 2019-08-17
- Use Moon orbital elements of 2019-08-11
- Added version management
- Added graphical effects on Sun/Kerbol: glow and lens flare
- Added CHANGELOG
- Removed obsolete console message when changing focus or timewarp
- Added numeric simulation of a simple space ship displayed as a 2km sphere
- Timewarp automatically plateaus when CPU is saturated
- Changed HUD to display distance from focus and altitude instead of zoom
- Show basic rocket as 2D texture
- Zoom is now limited by surface of focused object
- View rotation speed during drag-and-drop more natural when zoomed in
- Picking now prioritize object closest to mouse cursor
- Zoom factor changed to one decibel per scroll wheel tick
- Add support for cubemap textures

# v0.2.0 - 2019-08-09
- Added position markers for celestial bodies
- Added apses markers 
- Added anti-aliasing
- Show planets without textures as monochromatic spheres
- Enabled OpenGL error reporting in console
- Added keybinding to hide orbits and position markers
- Updated Solar system data from NASA
- Fixed Moon inclination
- Updated gravitational constant
- Added HUD
- Added keybinding for wireframe mode
- Added keybinding for help message
- Added Windows support

# v0.1.0 - 2019-08-05
- Rewrite spyce GUI in C++ (basic features):
    - Draw celestial bodies at their relative positions
    - Draw orbits
    - Draw skybox
    - Basic lighting
    - Time simulation and key bindings
    - Axial tilt
    - Rotation
    - Accurate rendering of orbits when focusing on planet
    - Accurate rendering of orbits when focusing on moon
    - Ability to switch focus by clicking

# v0.0.0 - 2019-07-12
- Rewrite spyce core in C
