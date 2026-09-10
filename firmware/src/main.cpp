#include <Arduino.h>
#include <driver/i2s.h>
#include <Voice-Activator-For-EdgeDevices-SIH26172_inferencing.h>

// -----------------------------
// ESP32 + INMP441 configuration
// -----------------------------
#define I2S_PORT I2S_NUM_0

#define I2S_BCLK 26
#define I2S_LRCLK 25
#define I2S_DIN 33

#define LED_PIN 2

#define SAMPLE_RATE 16000
#define NUM_SAMPLES EI_CLASSIFIER_RAW_SAMPLE_COUNT

static int16_t audio_buffer[NUM_SAMPLES];

// -----------------------------
// I2S setup
// -----------------------------
void setupI2S()
{
    Serial.println("===== SIH26172 VOICE ACTIVATOR =====");

    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(
            I2S_MODE_MASTER |
            I2S_MODE_RX
        ),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 256,
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0
    };

    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_BCLK,
        .ws_io_num = I2S_LRCLK,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = I2S_DIN
    };

    esp_err_t result;

    result = i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
    if (result != ESP_OK) {
        Serial.printf("ERROR: i2s_driver_install failed: %d\n", result);
        while (true) { delay(1000); }
    }

    result = i2s_set_pin(I2S_PORT, &pin_config);
    if (result != ESP_OK) {
        Serial.printf("ERROR: i2s_set_pin failed: %d\n", result);
        while (true) { delay(1000); }
    }

    i2s_zero_dma_buffer(I2S_PORT);

    Serial.println("I2S microphone initialized.");
}

// -----------------------------
// Capture exactly 1 second
// -----------------------------
bool captureAudio()
{
    Serial.println();
    Serial.println("Listening for 1 second...");

    i2s_zero_dma_buffer(I2S_PORT);

    int samples_collected = 0;

    while (samples_collected < NUM_SAMPLES)
    {
        int32_t raw_samples[256];
        size_t bytes_read = 0;

        esp_err_t result = i2s_read(I2S_PORT, raw_samples, sizeof(raw_samples), &bytes_read, portMAX_DELAY);

        if (result != ESP_OK) {
            Serial.printf("I2S read error: %d\n", result);
            return false;
        }

        int samples_read = bytes_read / sizeof(int32_t);

        for (int i = 0; i < samples_read && samples_collected < NUM_SAMPLES; i++)
        {
            int32_t sample = raw_samples[i];
            sample = sample >> 14;

            if (sample > 32767) sample = 32767;
            if (sample < -32768) sample = -32768;

            audio_buffer[samples_collected++] = (int16_t)sample;
        }
    }

    Serial.printf("Captured %d samples\n", samples_collected);

    int16_t minVal = 32767, maxVal = -32768;
    for (int i = 0; i < NUM_SAMPLES; i++) {
        if (audio_buffer[i] < minVal) minVal = audio_buffer[i];
        if (audio_buffer[i] > maxVal) maxVal = audio_buffer[i];
    }
    Serial.printf("Signal range: %d\n", maxVal - minVal);

    return true;
}

// -----------------------------
// Edge Impulse audio callback
// -----------------------------
static int getData(size_t offset, size_t length, float *out_ptr)
{
    for (size_t i = 0; i < length; i++)
    {
        out_ptr[i] = (float)audio_buffer[offset + i];
    }
    return 0;
}

// -----------------------------
// Run inference
// -----------------------------
void runInference()
{
    signal_t signal;
    signal.total_length = NUM_SAMPLES;
    signal.get_data = getData;

    Serial.println("Running Edge Impulse inference...");

    ei_impulse_result_t result = { 0 };

    EI_IMPULSE_ERROR res = run_classifier(&signal, &result, false);

    if (res != EI_IMPULSE_OK)
    {
        Serial.printf("Classifier failed: %d\n", res);
        return;
    }

    Serial.println();
    Serial.println("===== RESULTS =====");

    float best_score = 0.0f;
    const char *best_label = "unknown";

    for (size_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++)
    {
        float score = result.classification[i].value;
        const char *label = result.classification[i].label;

        Serial.printf("%s: %.3f\n", label, score);

        if (score > best_score)
        {
            best_score = score;
            best_label = label;
        }
    }

    Serial.printf("Best: %s (%.3f)\n", best_label, best_score);

    if (strcmp(best_label, "TALOS") == 0 && best_score >= 0.40f)
    {
        Serial.println();
        Serial.println("***** WAKE WORD DETECTED *****");

        digitalWrite(LED_PIN, HIGH);
        delay(1000);
        digitalWrite(LED_PIN, LOW);
    }
    else
    {
        Serial.println("No wake word detected.");
    }

    Serial.println("===================");
}

// -----------------------------
// Arduino setup
// -----------------------------
void setup()
{
    Serial.begin(115200);
    delay(2000);

    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    Serial.println();
    Serial.println("================================");
    Serial.println(" SIH26172 VOICE ACTIVATOR");
    Serial.println(" ESP32 + INMP441 + Edge Impulse");
    Serial.println("================================");

    Serial.printf("Expected samples: %d\n", NUM_SAMPLES);

    setupI2S();

    Serial.println();
    Serial.println("System ready.");
}

// -----------------------------
// Main loop
// -----------------------------
void loop()
{
    if (captureAudio())
    {
        runInference();
    }

    delay(200);
}
