#include "texture.h"
#include <framework/image.h>
#include "iostream"

glm::vec3 acquireTexel(const Image& image, const glm::vec2& texCoord, const Features& features)
{
    // TODO: implement this function.
    // Given texcoords, return the corresponding pixel of the image
    // The pixel are stored in a 1D array of row major order
    // you can convert from position (i,j) to an index using the method seen in the lecture
    // Note, the center of the first pixel is at image coordinates (0.5, 0.5)
    int i = std::float_round_style(texCoord.x * image.width);
    int j = std::float_round_style(image.height - texCoord.y * image.height);
    return image.pixels[j * image.width + i];
}