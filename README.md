# A miniature game engine created by niezhe

## Homework 6
### TODO
[] Change the cache of texture storage to a vector style. Everywhere need to hold the texture, give it a index of texture in the vector. (The lifecycle of the vector is equal to the program!!)

[] When calling RenderEx, check whether the image dst rect is in the region of window (How would I deal with the case of rotation?). If it's not in, don't render it. 

[] as the movement is continous and the speed is even be able to set in game.config, pay attention to how camera zoom change the movement scale on original grid.  
ab