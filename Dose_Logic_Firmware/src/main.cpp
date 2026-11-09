#include <Wire.h>
#include <SPI.h>
#include <PCA9557.h>

#include "gfx_conf.h"

PCA9557 Out;
LGFX_Sprite animation(&tft);

// Appearance
const uint16_t BACKGROUND_COLOR = TFT_RED;
const char* LOGO_TEXT = "Dose Logic!";
float targetDose = 12.5f;

// Animation
int logoX = 0;
int logoY = 0;
int speedX = 3;
int speedY = 2;

int logoWidth = 0;
int logoHeight = 0;

bool animationReady = false;
uint32_t lastFrame = 0;
const uint32_t FRAME_INTERVAL = 16; // Approximately 60 FPS

// Draw onto either the display or an off-screen sprite.
// Offsets let us redraw just a small portion of the screen.
void drawDoseCard(
  float dose,
  lgfx::LGFXBase& canvas,
  int offsetX = 0,
  int offsetY = 0
)
{
  int x = 20 - offsetX;
  int y = 90 - offsetY;
  int w = tft.width() - 40;
  int h = 150;

  uint16_t cardColor  = tft.color565(25, 35, 50);
  uint16_t labelColor = tft.color565(160, 175, 190);

  canvas.fillRoundRect(x, y, w, h, 16, cardColor);

  canvas.setTextSize(1);
  canvas.setTextDatum(top_left);

  canvas.setFont(&fonts::FreeSans12pt7b);
  canvas.setTextColor(labelColor, cardColor);
  canvas.drawString("Target dose", x + 24, y + 20);

  canvas.setFont(&fonts::FreeSans24pt7b);
  canvas.setTextColor(TFT_WHITE, cardColor);
  canvas.drawString(
    String(dose, 1) + " mL",
    x + 24,
    y + 65
  );
}

void drawMovingArea(int areaX, int areaY)
{
  // Rebuild the background inside this small sprite.
  animation.fillSprite(BACKGROUND_COLOR);

  // Draw the moving text in screen-relative coordinates.
  animation.setFont(&fonts::FreeSansBold18pt7b);
  animation.setTextSize(1);
  animation.setTextDatum(middle_center);
  animation.setTextColor(TFT_WHITE, BACKGROUND_COLOR);

  animation.drawString(
    LOGO_TEXT,
    logoX - areaX + logoWidth / 2,
    logoY - areaY + logoHeight / 2
  );

  // Draw the card last, so the moving text passes behind it.
  drawDoseCard(targetDose, animation, areaX, areaY);

  // Transfer the completed region to the screen.
  animation.pushSprite(areaX, areaY);
}

void setup()
{
  Serial.begin(115200);

  #if defined(CrowPanel_50) || defined(CrowPanel_70)
  // Touch timing initialization
  Wire.begin(19, 20);

  Out.reset();
  Out.setMode(IO_OUTPUT);
  Out.setState(IO0, IO_LOW);
  Out.setState(IO1, IO_LOW);

  delay(20);

  Out.setState(IO0, IO_HIGH);

  delay(100);/''

  Out.setMode(IO1, IO_INPUT);
/  #endif

  tft.begin();
  tft.fillScreen(BACKGROUND_COLOR);

  drawDoseCard(targetDose, tft);

  // Measure the logo using its actual font.
  animation.setColorDepth(16);
  animation.setFont(&fonts::FreeSansBold18pt7b);
  animation.setTextSize(1);

  logoWidth  = animation.textWidth(LOGO_TEXT) + 16;
  logoHeight = animation.fontHeight() + 12;

  if (logoWidth >= tft.width() ||
      logoHeight >= tft.height()) {
    Serial.println("Logo is too large for the screen");
    return;
  }

  // Enough space for both the old and new logo positions.
  // This avoids needing a full-screen framebuffer.
  int spriteWidth  = logoWidth + abs(speedX);
  int spriteHeight = logoHeight + abs(speedY);

  if (animation.createSprite(spriteWidth, spriteHeight) == nullptr) {
    Serial.println("Animation sprite allocation failed");
    return;
  }

  logoX = (tft.width() - logoWidth) / 2;
  logoY = tft.height() - logoHeight - 20;

  animationReady = true;
  drawMovingArea(logoX, logoY);
}

void loop()
{
  if (!animationReady) {
    delay(10);
    return;
  }

  uint32_t now = millis();

  if (now - lastFrame < FRAME_INTERVAL) {
    delay(1);
    return;
  }

  lastFrame = now;

  int oldX = logoX;
  int oldY = logoY;

  logoX += speedX;
  logoY += speedY;

  int maxX = tft.width() - logoWidth;
  int maxY = tft.height() - logoHeight;

  // Bounce off the left and right edges.
  if (logoX <= 0) {
    logoX = 0;
    speedX = abs(speedX);
  } else if (logoX >= maxX) {
    logoX = maxX;
    speedX = -abs(speedX);
  }

  // Bounce off the top and bottom edges.
  if (logoY <= 0) {
    logoY = 0;
    speedY = abs(speedY);
  } else if (logoY >= maxY) {
    logoY = maxY;
    speedY = -abs(speedY);
  }

  // Redraw a region covering both the old and new positions.
  int areaX = (oldX < logoX) ? oldX : logoX;
  int areaY = (oldY < logoY) ? oldY : logoY;

  drawMovingArea(areaX, areaY);
}