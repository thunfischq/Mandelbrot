#include <SFML/Graphics.hpp>
#include <iostream>
#include <cstdlib>
#include <math.h>
#include <algorithm>


sf::Color black(0, 0, 0);
sf::Color red(160, 16, 0);
sf::Color orange(222, 100, 16);
sf::Color yellow(232, 226, 42);
sf::Color green(100, 240, 34);
sf::Color turkise(33, 222, 177);
sf::Color blue(34, 69, 207);
sf::Color purple(69, 16, 160);


// Simple complex number struct consisting of two floats, being the real and the
// imaginary part.
struct Complex {
    float real;
    float imag;
};


// Initialize complex number by defining its real and imaginary part.
void initComplex(struct Complex *complex, float real, float imag) {
    complex->real = real;
    complex->imag = imag;
}


// This maybe could be done more efficient using sinus and tangent and shit. That way,
// the costly sqrt() could be evaded. But don't know how efficient sin and tan are...
// But this would be the main place for improved efficiency I think.
// The absolute value of a complex number, being the distance from (0, 0) in the plain.
float abs(Complex *complex) {
    return sqrt(pow(complex->real, 2) + pow(complex->imag, 2));
}


// Square a complex number.
Complex squareComplex(Complex *complex) {
    Complex newComplex;
    float real = pow(complex->real, 2) - pow(complex->imag, 2);
    float imag = 2 * complex->real * complex->imag;
    initComplex(&newComplex, real, imag);
    return newComplex;
}


// Add two complex numbers.
Complex addComplex(Complex *a, Complex *b) {
    Complex c;
    initComplex(&c, a->real + b->real, a->imag + b->imag);
    return c;
}


// Run the "mandelbro sequence" on given complex number until it explodes or the amount
// of max loops is reached.
int mandelbrot(Complex *c, int maxLoops) {
    int amountLoops = 0;
    Complex sumSequence;
    initComplex(&sumSequence, 0, 0);
    while (amountLoops < maxLoops && abs(&sumSequence) < 2) {
        Complex squared = squareComplex(&sumSequence);
        sumSequence = addComplex(&squared, c);
        amountLoops++;
    }
    return amountLoops;
}


sf::Color interpolate(sf::Color colorA, sf::Color colorB, float t) {
    sf::Color interpolatedColor(
            colorA.r + static_cast<sf::Uint8>(t * (colorB.r - colorA.r)),
            colorA.g + static_cast<sf::Uint8>(t * (colorB.g - colorA.g)),
            colorA.b + static_cast<sf::Uint8>(t * (colorB.b - colorA.b))
        );
    return interpolatedColor;
}


// Creates a color transition: black - red - orange - yellow - green - turkise - blue
sf::Color color(int i, int maxI, float sixMaxI) {
    float t;
    if ((6 * i) < maxI) {
        t = i * sixMaxI;
        return interpolate(blue, turkise, t);
    }
    else if ((3 * i) < maxI) {
        t = i * sixMaxI - 1;
        return interpolate(turkise, green, t);
    }
    else if ((2 * i) < maxI) {
        t = i * sixMaxI - 2;
        return interpolate(green, yellow, t);
    }
    else if ((3 * i) < 2 * maxI) {
        t = i * sixMaxI - 3;
        return interpolate(yellow, orange, t);
    }
    else if ((6 * i) < (5 * maxI)) {
        t = i * sixMaxI - 4;
        return interpolate(orange, red, t);
    }
    else {
        t = i * sixMaxI - 5;
        return interpolate(red, black, t);
    }
}


// Calculate the color of a given pixel (x, y) by finding its complex value using the
// upperLeft and lowerRight complex number anchors and the width and height of the
// window, then running the "mandelbrot sequence" on it and calculating the color by the
// amount of loops mandelbrot() returns.
sf::Color getPixelColor(int x, int y, Complex *upperLeft, Complex *lowerRight, int width, int height, int maxI, float sixMaxI) {
    Complex c;
    float ratioX = (float)x / width;
    float ratioY = (float)y / height;
    initComplex(&c, upperLeft->real + (ratioX * (lowerRight->real - upperLeft->real)),
                    upperLeft->imag + (ratioY * (lowerRight->imag - upperLeft->imag)));
    return color(mandelbrot(&c, maxI), maxI, sixMaxI);
}


// Zoom into the position of the cursor by 1/10 by moving the upperLeft and lowerRight
// complex number anchors towards the cursor position by 1/10 of their distance.
void zoomIn(int x, int y, Complex *upperLeft, Complex *lowerRight, int width, int height, float zoomFactor) {
    Complex position;
    float ratioX = (float)x / width;
    float ratioY = (float)y / height;
    initComplex(&position, upperLeft->real + (ratioX * (lowerRight->real - upperLeft->real)),
                    upperLeft->imag + (ratioY * (lowerRight->imag - upperLeft->imag)));
    upperLeft->real = upperLeft->real + (((float)1/10) * (position.real - upperLeft->real));
    upperLeft->imag = upperLeft->imag + (((float)1/10) * (position.imag - upperLeft->imag));
    lowerRight->real = lowerRight->real + (((float)1/10) * (position.real - lowerRight->real));
    lowerRight->imag = lowerRight->imag + (((float)1/10) * (position.imag - lowerRight->imag));
}


int main(int argc, char* argv[]) {
    int width = 800;
    int height = 600;
    int maxI = 100;
    float sixDivMaxI = (float)6 / maxI; // used to make color calculation more efficient
    bool update = true;
    bool sharpen = false;
    bool blur = false;
    Complex upperLeft;
    Complex lowerRight;
    initComplex(&upperLeft, -2.5, 1);
    initComplex(&lowerRight, 1, -1);

    // optional terminal arguments
    if (argc > 1) {
        width = std::atoi(argv[1]);
    }
    if (argc > 2) {
        height = std::atoi(argv[2]);
    }
    if (argc > 3) {
        maxI = std::max(std::atoi(argv[3]), 100);
    }

    sf::RenderWindow window(sf::VideoMode(width, height), "Mandelbrot");
    sf::Image image;
    image.create(width, height, sf::Color::Black);

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
            // sharpen and blur image based on keyboard inputs
            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == 47 && sharpen == false) {
                    maxI += 50;
                    sixDivMaxI = (float)6 / maxI;
                    update = true;
                    sharpen = true;
                }
                if (event.key.code == 56 && blur == false) {
                    maxI = std::max(100, maxI - 50);
                    sixDivMaxI = (float)6 / maxI;
                    update = true;
                    blur = true;
                }
                if (event.key.code == sf::Keyboard::Escape) {
                    window.close();
                }
            }
            if (event.type == sf::Event::KeyReleased) {
                if (event.key.code == 47) {
                    sharpen = false;
                }
                if (event.key.code == 56) {
                    blur = false;
                }
            }
        }

        if (update) {
            // determine color for all pixels and set them in image
            for (int x = 0; x < width; ++x) {
                for (int y = 0; y < height; ++y) {
                    sf::Color color = getPixelColor(x, y, &upperLeft, &lowerRight, width, height, maxI, sixDivMaxI);
                    image.setPixel(x, y, color);
                }
            }

            sf::Texture texture;
            texture.loadFromImage(image);
            sf::Sprite sprite(texture);

            // clear previous image and draw new one
            window.clear();
            window.draw(sprite);
            window.display();
            update = false;
        }

        // call zoomIn function if mouse left button was pressed whilst the cursor is
        // inside the program window
        sf::Vector2i mousePosition = sf::Mouse::getPosition(window);
        if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
            if (mousePosition.x >= 0 && mousePosition.x < width && mousePosition.y >= 0 && mousePosition.y < height) {
                zoomIn(mousePosition.x, mousePosition.y, &upperLeft, &lowerRight, width, height, 0.9);
                update = true;
            }
        }
    }

    return 0;
}
