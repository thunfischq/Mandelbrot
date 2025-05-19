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


// Converts HSV values to an sf::Color in RGB space
sf::Color hsvToRgb(float h, float s, float v) {
    float c = v * s; // Chroma
    float x = c * (1 - std::fabs(std::fmod(h / 60.0f, 2) - 1));
    float m = v - c;

    float r, g, b;
    if (h < 60) {r = c; g = x; b = 0; }
    else if (h < 120){ r = x; g = c; b = 0; }
    else if (h < 180){ r = 0; g = c; b = x; }
    else if (h < 240){ r = 0; g = x; b = c; }
    else if (h < 300){ r = x; g = 0; b = c; }
    else             { r = c; g = 0; b = x; }

    return sf::Color(
        static_cast<sf::Uint8>((r + m) * 255),
        static_cast<sf::Uint8>((g + m) * 255),
        static_cast<sf::Uint8>((b + m) * 255)
    );
}


sf::Color color(int i, int maxI) {
    if (i >= maxI) {
        return sf::Color::Black; // Inside the set
    }

    // Normalized progress (0 to 1)
    float t = static_cast<float>(i) / 500;

    // Apply nonlinear transformation to compress high values
    t = std::pow(t, 0.9f); // (0.5 = strong stretch, 1.0 = linear)

    // Repeat hue multiple times
    float hue = fmod(360.0f * t * 1.2f, 360.0f);

    float saturation = 0.8f;
    float value = 0.9f;

    return hsvToRgb(hue, saturation, value);
}


// Calculate the color of a given pixel (x, y) by finding its complex value using the
// upperLeft and lowerRight complex number anchors and the width and height of the
// window, then running the "mandelbrot sequence" on it and calculating the color by the
// amount of loops mandelbrot() returns.
sf::Color getPixelColor(int x, int y, const Complex* upperLeft, const Complex* lowerRight,
                        int width, int height, int maxI) {
    Complex c;
    double ratioX = (double)x / width;
    double ratioY = (double)y / height;
    initComplex(&c, upperLeft->real + (ratioX * (lowerRight->real - upperLeft->real)),
                    upperLeft->imag + (ratioY * (lowerRight->imag - upperLeft->imag)));
    return color(mandelbrot(&c, maxI), maxI);
}


// Compute rows of pixel in an interleaved pattern for better performance
void computeRowsInterleaved(int threadId, int threadCount, int width, int height,
    const Complex& upperLeft, const Complex& lowerRight, int maxI,
    std::vector<std::vector<sf::Color>>& rowBuffer) {
    // rows are allocated in a module sense, so if there are f.e. 3 workers,
    // worker one will get row 1, 4, 7,... and the other workers accordingly
    for (int y = threadId; y < height; y += threadCount) {
        for (int x = 0; x < width; ++x) {
            sf::Color color = getPixelColor(x, y, &upperLeft, &lowerRight,
                                            width, height, maxI);
            rowBuffer[y][x] = color;
        }
    }
}


// Divide the rows that are to be computed and allocate them to the workers
void divideAndConquer(const Complex* upperLeft, const Complex* lowerRight,
                            int width, int height, int maxI,
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
                            std::ref(rowBuffer));
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


// Zoom into the position of the cursor by zoomFactor by moving the upperLeft and
// lowerRight complex number anchors towards the cursor position by zoomFactor of
// their distance.
void zoomInCursor(int x, int y, Complex *upperLeft, Complex *lowerRight,
                    int width, int height, double zoomFactor) {
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


// Zoom into the auto zoom target coordinates by zoomFactor by moving the upperLeft and
// lowerRight complex number anchors towards the cursor position by zoomFactor of their
// distance.
void zoomInAuto(Complex *target, Complex *upperLeft, Complex *lowerRight, double zoomFactor) {
    upperLeft->real = upperLeft->real + (zoomFactor * (target->real - upperLeft->real));
    upperLeft->imag = upperLeft->imag + (zoomFactor * (target->imag - upperLeft->imag));
    lowerRight->real = lowerRight->real + (zoomFactor * (target->real - lowerRight->real));
    lowerRight->imag = lowerRight->imag + (zoomFactor * (target->imag - lowerRight->imag));
}


// update debug text containing the coordinates of the cursor as (real, imag) and
// update the backgroud text box
void updateTextRender(Complex *upperLeft, Complex *lowerRight, sf::Text* debugText,
                        sf::FloatRect* textBounds, sf::RectangleShape* background,
                        double* mouseReal, double* mouseImag, int mouseX, int mouseY,
                        int windowWidth, int windowHeight, int maxI, double zoomFactor,
                        bool save, bool zoom, bool screenshot) {
    
    // compute real and imag coordinates of cursor
    (*mouseReal) = upperLeft->real + ((mouseX /
                    static_cast<double>(windowWidth))
                    * (lowerRight->real - upperLeft->real));
    (*mouseImag) = upperLeft->imag + ((mouseY /
                    static_cast<double>(windowHeight))
                    * (lowerRight->imag - upperLeft->imag));
    
    // write coordinates into debug text
    std::ostringstream cords;
    cords << std::fixed << std::setprecision(16) << (*mouseReal) << "\n"
            << std::fixed << std::setprecision(16) << (*mouseImag) << "\n"
            << "Max iterations: " << maxI << "\n" << "Zoom factor: "
            << std::fixed << std::setprecision(3) << zoomFactor << "\n"
            << "Autozoom: " << zoom << " | Saving: " << save;
    if (screenshot) {
        cords << "\nScreenshot saved.";
    }
    (*debugText).setString(cords.str());

    // update text box
    (*textBounds) = (*debugText).getLocalBounds();
    (*background).setSize(sf::Vector2f((*textBounds).width + 10,
                            (*textBounds).height + 15));
    (*background).setFillColor(sf::Color::Black);
    (*background).setPosition((*debugText).getPosition().x - 5,
                                (*debugText).getPosition().y - 5);
}


int main(int argc, char* argv[]) {
    int width = 1280;
    int height = 720;

    int mouseX;
    int mouseY;
    int windowWidth;
    int windowHeight;
    double mouseReal;
    double mouseImag;
    int mouseOldX = -1;
    int mouseOldY = -1;

    int maxI = 100;
    bool update = true;
    bool updateText = true;
    bool sharpen = false;
    bool blur = false;
    bool autoZoom = false;
    bool saveFrames = false;
    bool fullscreen = false;
    bool renderText = false;
    bool screenShot = false;
    uint32_t frameCounter = 0;
    int maxFrames = -1;
    double zoomFactor = 0.1;

    Complex upperLeft;
    Complex lowerRight;
    Complex autoZoomTarget; // horsesea valley
    initComplex(&upperLeft, -2.5, 1);
    initComplex(&lowerRight, 1, -1);
    initComplex(&autoZoomTarget, -0.7435849988388146, 0.1318760846245895);

    std::string helpText = "Give no optional arguments for a 1280x720 rendering.\n\nUse LMB to zoom into the position of the cursor, press SPACE to toggle auto zoom, f to toggle fullscreen and t to toggle debug text. Press s to take a screenshot.\n\nUse:\n-r WIDTH HEIGHT for custom resolution (anything different from 16:9 will be distorted!) [Standard 1280 720]\n-c REAL IMAG for custom zoom coordinates [Standard -0.7435... 0.1318...]\n-m MAX for a maximum amount of frames before the program auto closes [Standard -1]\n-a to enable auto zoom from the beginning\n-s to save the frames as png's in /frames\n-i MAXI to change the maximum amount of iterations before a pixel is considered black [Standard 100]\n-z ZOOMFACTOR to change how much to zoom in for each new frame [Standard 0.1]\n-f to enable fullscreen at startup\n-t to enable debug text at startup\n";

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
            i++;
        } else if (std::strcmp(argv[i], "-z") == 0) {
            zoomFactor = std::atof(argv[i + 1]);
            i++;
        } else if (std::strcmp(argv[i], "-f") == 0) {
            fullscreen = true;
        } else if (std::strcmp(argv[i], "-t") == 0) {
            renderText = true;
        } else if (std::strcmp(argv[i], "-c") == 0) {
            autoZoomTarget.real = std::atof(argv[i + 1]);
            autoZoomTarget.imag = std::atof(argv[i + 2]);
            i++;
            i++;
        } else {
            std::cout << helpText;
            exit(0);
        }
    }

    // initiate renderer
    sf::VideoMode desktopFull = sf::VideoMode::getDesktopMode();
    sf::VideoMode videoMode = fullscreen ? sf::VideoMode::getDesktopMode()
                                     : sf::VideoMode(width, height);
    sf::Uint32 style = fullscreen ? sf::Style::None: sf::Style::Default;
    sf::RenderWindow window(videoMode, "Mandelbrot", style);

    if (fullscreen) {
        window.setPosition(sf::Vector2i(0, 0));
    }

    // initiate screen scaling for fullscreen
    sf::Image image;
    image.create(width, height);
    int screenWidth = desktopFull.width;
    int screenHeight = desktopFull.height;
    double scaleX = static_cast<double>(screenWidth) / width;
    double scaleY = static_cast<double>(screenHeight) / height;

    // initiate font
    sf::Font font;
    if (!font.loadFromFile(".Ubuntu-R.ttf")) {
        std::cout << "Problem loading font!\n";
    }

    // initiate text
    sf::Text debugText;
    debugText.setFont(font);
    debugText.setCharacterSize(16);
    debugText.setFillColor(sf::Color::White);
    debugText.setPosition(5, 5);
    debugText.setString("Not supposed to see this :)");
    sf::FloatRect textBounds = debugText.getLocalBounds();
    sf::RectangleShape background;

    // main loop
    while (window.isOpen()) {
        // handle keyboard input
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
            // sharpen and blur image based on keyboard inputs
            if (event.type == sf::Event::KeyPressed) {
                // + sharpens the image by increasing maxI
                if (event.key.code == 47 && sharpen == false) {
                    maxI += 10;
                    update = true;
                    sharpen = true;
                }
                // - blurs the image by decreasing maxI
                if (event.key.code == 56 && blur == false) {
                    maxI = std::max(100, maxI - 10);
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
                // t toggles debug text rendering
                if (event.key.code == 19) {
                    renderText = !renderText;
                    updateText = true;
                }
                // f toggles fullscreen
                if (event.key.code == 5) {
                    fullscreen = !fullscreen;

                    window.close();
                    sf::VideoMode newMode = fullscreen ? sf::VideoMode::getDesktopMode()
                                                        : sf::VideoMode(width, height);
                    sf::Uint32 newStyle = fullscreen ? sf::Style::None
                                                        : sf::Style::Default;
                    window.create(newMode, "Mandelbrot", newStyle);
                    
                    if (fullscreen) {
                        window.setPosition(sf::Vector2i(0, 0));
                    }
                    
                    update = true;
                }
                // s takes a screenshot
                if (event.key.code == 18) {
                    if (!saveFrames) {
                        screenShot = true;
                    }
                }
                // close window if esc pressed
                if (event.key.code == sf::Keyboard::Escape) {
                    window.close();
                }
            }
            // probably not needed but oh well
            if (event.type == sf::Event::KeyReleased) {
                if (event.key.code == 47) {
                    sharpen = false;
                }
                if (event.key.code == 56) {
                    blur = false;
                }
            }
        }

        // update mouse and window position / dimension
        sf::Vector2i mousePosition = sf::Mouse::getPosition(window);
        mouseX = mousePosition.x;
        mouseY = mousePosition.y;
        if (fullscreen) {
            windowWidth = screenWidth;
            windowHeight = screenHeight;
        } else {
            windowWidth = width;
            windowHeight = height;
        }

        // update text if mouse has been moved
        // this way, text is updated even when sprite isn't
        if ((mouseX != mouseOldX || mouseY != mouseOldY)
            && mousePosition.x >= 0 && mousePosition.x < windowWidth
            && mousePosition.y >= 0 && mousePosition.y < windowHeight) {

            mouseOldX = mouseX;
            mouseOldY = mouseY;
            updateText = true;
        }

        // don't need to update render for screenshot
        if (screenShot) {
            std::ostringstream filename;
            filename << "frames/frame_" << std::setw(4) << std::setfill('0')
                        << frameCounter << ".png";
            image.saveToFile(filename.str());
            std::cout << "Saved screenshot to: " << filename.str() << "\n";
            screenShot = false;
        }

        // update rendering
        if (update) {
            frameCounter++;
            //if (frameCounter % 1 == 0) {
            //    maxI += 1;
            //}

            // main mandelbrot update logic
            divideAndConquer(&upperLeft, &lowerRight, width, height, maxI, &image);

            // update text box logic
            if (renderText) {
                updateTextRender(&upperLeft, &lowerRight, &debugText, &textBounds,
                                &background, &mouseReal, &mouseImag, mouseX, mouseY,
                                windowWidth, windowHeight, maxI, zoomFactor, saveFrames,
                                autoZoom, false);
            }
            
            // store frame as png
            if (saveFrames) {
                std::ostringstream filename;
                filename << "frames/frame_" << std::setw(4) << std::setfill('0')
                            << frameCounter << ".png";
                image.saveToFile(filename.str());
            }

            sf::Texture texture;
            sf::Sprite sprite;

            texture.loadFromImage(image);
            texture.setSmooth(true);
            sprite.setTexture(texture);

            // scale sprite with dimensions (width, height) to (screenWidth, screenHeight)
            if (fullscreen) {
                sprite.setScale(scaleX, scaleY);
            }

            // clear previous image and draw new one
            window.clear();
            window.draw(sprite);
            if (renderText) {
                window.draw(background);
                window.draw(debugText);
            }
            window.display();

            if (!autoZoom) {
                update = false;
            }

            if (maxFrames > 0 && maxFrames <= frameCounter) {
                window.close();
            }
        // only update debug text, still need to draw the rest
        } else if (updateText) {  
            // update text box logic  
            if (renderText) {
                updateTextRender(&upperLeft, &lowerRight, &debugText, &textBounds,
                                &background, &mouseReal, &mouseImag, mouseX, mouseY,
                                windowWidth, windowHeight, maxI, zoomFactor, saveFrames,
                                autoZoom, false);
            }

            sf::Texture texture;
            sf::Sprite sprite;

            texture.loadFromImage(image);
            texture.setSmooth(true);
            sprite.setTexture(texture);
            
            // scale sprite with dimensions (width, height) to (screenWidth, screenHeight)
            if (fullscreen) {
                sprite.setScale(scaleX, scaleY);
            }

            // clear previous image and draw new one
            window.clear();
            window.draw(sprite);
            if (renderText) {
                window.draw(background);
                window.draw(debugText);
            }
            window.display();
            updateText = false;
        }

        // call zoom function if LMB was pressed whilst the cursor is
        // inside the program window
        if (sf::Mouse::isButtonPressed(sf::Mouse::Left) && !autoZoom) {
            if (mousePosition.x >= 0 && mousePosition.x < windowWidth
                && mousePosition.y >= 0 && mousePosition.y < windowHeight) {
                zoomInCursor(fullscreen ? mouseX / scaleX : mouseX,
                            fullscreen ? mouseY / scaleY : mouseY,
                            &upperLeft, &lowerRight, width, height,
                            zoomFactor);
                update = true;
            }
        }
        // call autoZoom if enabled
        if (autoZoom) {
            zoomInAuto(&autoZoomTarget, &upperLeft, &lowerRight, zoomFactor);
        }
    }

    // print info when terminated
    std::cout << "Generated " << frameCounter << " frames.\n";
    std::cout << "Max iterations: " << maxI << "\n";
    std::ostringstream cordsReal;
    std::ostringstream cordsImag;
    cordsReal << std::fixed << std::setprecision(16) << mouseReal;
    cordsImag << std::fixed << std::setprecision(16) << mouseImag;
    std::cout << "Mouse real: " << cordsReal.str() << "\n";
    std::cout << "Mouse imag: " << cordsImag.str() << "\n";

    return 0;
}
