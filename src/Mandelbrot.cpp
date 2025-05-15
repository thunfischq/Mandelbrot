#include <SFML/Graphics.hpp>
#include <iostream>
#include <cstdlib>
#include <math.h>
#include <algorithm>
#include <thread>
#include <vector>
#include <sstream>
#include <iomanip>
#include <cstring>


sf::Color black(0, 0, 0);
sf::Color red(160, 16, 0);
sf::Color orange(222, 100, 16);
sf::Color yellow(232, 226, 42);
sf::Color green(100, 240, 34);
sf::Color turkise(33, 222, 177);
sf::Color blue(34, 69, 207);
sf::Color purple(69, 16, 160);


// Simple complex number struct consisting of two doubles, being the real and the
// imaginary part.
struct Complex {
    double real;
    double imag;
};


// Initialize complex number by defining its real and imaginary part.
void initComplex(struct Complex *complex, double real, double imag) {
    complex->real = real;
    complex->imag = imag;
}


// The absolute value of a complex number, being the distance from (0, 0) in the plain.
// Calculating SQUARED distance value sqrt(a^2+b^2)^2 = abs(a^2+b^2) instead of normal
// distance value sqrt(a^2+b^2) for efficiency.
double absSquared(Complex *complex) {
    return abs(pow(complex->real, 2) + pow(complex->imag, 2));
}


// Square a complex number.
Complex squareComplex(Complex *complex) {
    Complex newComplex;
    double real = pow(complex->real, 2) - pow(complex->imag, 2);
    double imag = 2 * complex->real * complex->imag;
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
    // normally the value "explodes" when abs >= 2, but because we have squared abs,
    // we abort if it gets >= 4 okayge.
    while (amountLoops < maxLoops && absSquared(&sumSequence) < 4) {
        Complex squared = squareComplex(&sumSequence);
        sumSequence = addComplex(&squared, c);
        amountLoops++;
    }
    return amountLoops;
}


// Don't know how this works, ChatGPT told me to do it like this :D
sf::Color interpolate(sf::Color colorA, sf::Color colorB, double t) {
    sf::Color interpolatedColor(
            colorA.r + static_cast<sf::Uint8>(t * (colorB.r - colorA.r)),
            colorA.g + static_cast<sf::Uint8>(t * (colorB.g - colorA.g)),
            colorA.b + static_cast<sf::Uint8>(t * (colorB.b - colorA.b))
        );
    return interpolatedColor;
}


// Creates a color transition: black - red - orange - yellow - green - turkise - blue
sf::Color color(int i, int maxI, double sixMaxI) {
    double t;
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
sf::Color getPixelColor(int x, int y, const Complex* upperLeft, const Complex* lowerRight,
                        int width, int height, int maxI, double sixMaxI) {
    Complex c;
    double ratioX = (double)x / width;
    double ratioY = (double)y / height;
    initComplex(&c, upperLeft->real + (ratioX * (lowerRight->real - upperLeft->real)),
                    upperLeft->imag + (ratioY * (lowerRight->imag - upperLeft->imag)));
    return color(mandelbrot(&c, maxI), maxI, sixMaxI);
}


// Compute rows of pixel in an interleaved pattern for better performance
void computeRowsInterleaved(int threadId, int threadCount, int width, int height,
    const Complex& upperLeft, const Complex& lowerRight, int maxI, double sixDivMaxI,
    std::vector<std::vector<sf::Color>>& rowBuffer) {
    // rows are allocated in a module sense, so if there are f.e. 3 workers,
    // worker one will get row 1, 4, 7,... and the other workers accordingly
    for (int y = threadId; y < height; y += threadCount) {
        for (int x = 0; x < width; ++x) {
            sf::Color color = getPixelColor(x, y, &upperLeft, &lowerRight,
                                            width, height, maxI, sixDivMaxI);
            rowBuffer[y][x] = color;
        }
    }
}


// Divide the rows that are to be computed and allocate them to the workers
void divideAndConquer(const Complex* upperLeft, const Complex* lowerRight,
                            int width, int height, int maxI, double sixDivMaxI,
                            sf::Image* image) {
    // number of threads based on CPU cores
    const int threadCount = std::thread::hardware_concurrency();
    std::vector<std::thread> threads;

    // create a 2D buffer for all pixels, initialized with the correct size
    std::vector<std::vector<sf::Color>> rowBuffer(height, std::vector<sf::Color>(width));

    // divide work among worker
    for (int i = 0; i < threadCount; ++i) {
        threads.emplace_back(computeRowsInterleaved, i, threadCount, width, height,
                             std::cref(*upperLeft), std::cref(*lowerRight), maxI,
                             sixDivMaxI, std::ref(rowBuffer));
    }

    // wait for all threads to finish
    for (auto& thread : threads) {
        thread.join();
    }

    // merge all pixel rows into an image
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            (*image).setPixel(x, y, rowBuffer[y][x]);
        }
    }
}


// Zoom into the position of the cursor by 1/10 by moving the upperLeft and lowerRight
// complex number anchors towards the cursor position by 1/10 of their distance.
void zoomInCursor(int x, int y, Complex *upperLeft, Complex *lowerRight, int width, int height, double zoomFactor) {
    Complex position;
    double ratioX = (double)x / width;
    double ratioY = (double)y / height;
    initComplex(&position, upperLeft->real + (ratioX * (lowerRight->real - upperLeft->real)),
                    upperLeft->imag + (ratioY * (lowerRight->imag - upperLeft->imag)));
    upperLeft->real = upperLeft->real + (zoomFactor * (position.real - upperLeft->real));
    upperLeft->imag = upperLeft->imag + (zoomFactor * (position.imag - upperLeft->imag));
    lowerRight->real = lowerRight->real + (zoomFactor * (position.real - lowerRight->real));
    lowerRight->imag = lowerRight->imag + (zoomFactor * (position.imag - lowerRight->imag));
}


void zoomInAuto(Complex *target, Complex *upperLeft, Complex *lowerRight, double zoomFactor) {
    upperLeft->real = upperLeft->real + (zoomFactor * (target->real - upperLeft->real));
    upperLeft->imag = upperLeft->imag + (zoomFactor * (target->imag - upperLeft->imag));
    lowerRight->real = lowerRight->real + (zoomFactor * (target->real - lowerRight->real));
    lowerRight->imag = lowerRight->imag + (zoomFactor * (target->imag - lowerRight->imag));
}


int main(int argc, char* argv[]) {
    int width = 1280;
    int height = 720;
    int maxI = 100;
    double sixDivMaxI = (double)6 / maxI; // used to make color calculation more efficient
    bool update = true;
    bool sharpen = false;
    bool blur = false;
    bool autoZoom = false;
    bool saveFrames = false;
    uint32_t frameCounter = 0;
    int maxFrames = -1;
    Complex upperLeft;
    Complex lowerRight;
    Complex autoZoomTarget;
    initComplex(&upperLeft, -2.5, 1);
    initComplex(&lowerRight, 1, -1);
    initComplex(&autoZoomTarget, -0.74364386269, 0.13182590271); // horsesea valley

    std::string helpText = "help text\n";

    // parse optional terminal arguments
    for (int i = 1; i < argc; i++) {
        if (std::strcmp(argv[i], "-r") == 0) {
            width = std::atoi(argv[i + 1]);
            height = std::atoi(argv[i + 2]);
            i++;
            i++;
        } else if (std::strcmp(argv[i], "-h") == 0) {
            std::cout << helpText;
            exit(0);
        } else if (std::strcmp(argv[i], "-m") == 0) {
            maxFrames = std::atoi(argv[i + 1]);
            i++;
        } else if (std::strcmp(argv[i], "-a") == 0) {
            autoZoom = true;
        } else if (std::strcmp(argv[i], "-s") == 0) {
            saveFrames = true;
        } else if (std::strcmp(argv[i], "-i") == 0) {
            maxI = std::atoi(argv[i + 1]);
            sixDivMaxI = (double)6 / maxI;
            i++;
        } else {
            std::cout << helpText;
            exit(0);
        }
    }

    // initiate renderer
    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
    sf::RenderWindow window(desktop, "Mandelbrot", sf::Style::None);

    // compute scaling
    sf::Image image;
    image.create(width, height);
    window.setPosition({0, 0});
    int screenWidth = desktop.width;
    int screenHeight = desktop.height;
    double scaleX = static_cast<double>(screenWidth) / width;
    double scaleY = static_cast<double>(screenHeight) / height;

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            // handle keyboard input
            if (event.type == sf::Event::Closed) {
                window.close();
            }
            // sharpen and blur image based on keyboard inputs
            if (event.type == sf::Event::KeyPressed) {
                // + sharpens the image by increasing maxI
                if (event.key.code == 47 && sharpen == false) {
                    maxI += 20;
                    sixDivMaxI = (double)6 / maxI;
                    update = true;
                    sharpen = true;
                }
                // - blurs the image by decreasing maxI
                if (event.key.code == 56 && blur == false) {
                    maxI = std::max(100, maxI - 20);
                    sixDivMaxI = (double)6 / maxI;
                    update = true;
                    blur = true;
                }
                // space enables auto-zoom
                if (event.key.code == 57) {
                    autoZoom = (!autoZoom);
                    if (autoZoom) {
                        update = true;
                    } else {
                        update = false;
                    }
                }
                // close window if esc pressed
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

        // update rendering if requested
        if (update) {
            frameCounter++;
            if (frameCounter % 2 == 0) {
                maxI += 1;
            }
            sixDivMaxI = (double)6 / maxI;

            divideAndConquer(&upperLeft, &lowerRight, width, height, maxI,
                            sixDivMaxI, &image);
            
            if (saveFrames) {
                std::ostringstream filename;
                filename << "frames/frame_" << std::setw(4) << std::setfill('0') << frameCounter << ".png";
                image.saveToFile(filename.str());
            }

            sf::Texture texture;
            sf::Sprite sprite;

            texture.loadFromImage(image);
            texture.setSmooth(true);
            sprite.setTexture(texture);

            // scale sprite with dimensions (width, height) to (screenWidth, screenHeight)
            sprite.setScale(scaleX, scaleY);

            // clear previous image and draw new one
            window.clear();
            window.draw(sprite);
            window.display();

            if (!autoZoom) {
                update = false;
            }

            if (maxFrames > 0 && maxFrames <= frameCounter) {
                window.close();
            }
        }

        // call zoom function if LMB was pressed whilst the cursor is
        // inside the program window
        sf::Vector2i mousePosition = sf::Mouse::getPosition(window);
        if (sf::Mouse::isButtonPressed(sf::Mouse::Left) && !autoZoom) {
            if (mousePosition.x >= 0 && mousePosition.x < screenWidth
                && mousePosition.y >= 0 && mousePosition.y < screenHeight) {
                zoomInCursor(mousePosition.x / scaleX, mousePosition.y / scaleY,
                        &upperLeft, &lowerRight, width, height, 0.1);
                update = true;
            }
        }
        if (autoZoom) {
            zoomInAuto(&autoZoomTarget, &upperLeft, &lowerRight, 0.01);
        }
    }

    std::cout << "Generated " << frameCounter << " frames.\n";
    std::cout << "MaxI " << maxI << "\n";

    return 0;
}
