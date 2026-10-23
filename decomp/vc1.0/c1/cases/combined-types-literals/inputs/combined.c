typedef unsigned short wchar_t;

enum Signal {
    signal_below_short = -32769,
    signal_negative = -3,
    signal_ready = 7,
    signal_wide = 70000,
    signal_limit = 2147483647
};

struct MeasurementLedger {
    enum Signal current_operating_signal;
    int primary_temperature_measurement;
    int secondary_temperature_measurement;
    int ambient_pressure_measurement;
    int reference_pressure_measurement;
    int minimum_observed_sample_value;
    int maximum_observed_sample_value;
    int previous_accumulated_sample_count;
    int current_accumulated_sample_count;
    int accepted_calibration_sample_count;
    int rejected_calibration_sample_count;
    int upper_warning_threshold_value;
    int lower_warning_threshold_value;
    int first_reserved_calibration_word;
    int second_reserved_calibration_word;
    int third_reserved_calibration_word;
    int fourth_reserved_calibration_word;
    int last_successful_sequence_number;
};

struct Quality {
    unsigned active : 1;
    unsigned channel : 5;
    signed adjustment : 6;
};
union NumericSample {
    double fraction;
    long integral[2];
};
struct Calibration {
    float scale;
    double origin, step;
    long integral_bias;
    union NumericSample sample;
    struct Quality quality;
};

struct MeasurementLedger ledger;
enum Signal selected_signal = signal_negative;
enum Signal wide_signal = signal_wide;
struct Calibration calibration = {
    (1.5f + 2.25f) * 2.0f,
    (9.0 - 1.0) / 4.0,
    -((3.0 * 5.0) / 2.0),
    (long)(12.75 - 2.0),
    { 1.0 / 8.0 },
    { 1, 17, -5 }
};

char narrow_message[] = "start\t" "\101\x42" ":\a\b\f\n\r\v\\\"\'\?" "end";
wchar_t wide_message[] = L"wide\t" L"\x03a9\123" L":done";
char continued_message[] = "first\
second" ":third";
unsigned char escaped_character = (unsigned char)'\377';

static enum Signal choose_signal(enum Signal requested, int enabled)
{
    if (!enabled) return signal_negative;
    if (requested == signal_wide) return requested;
    return signal_ready;
}

typedef enum Signal (*SignalSelector)(enum Signal, int);
SignalSelector selector = choose_signal;

int main(void)
{
    ledger.current_operating_signal = selector(wide_signal, 1);
    if (ledger.current_operating_signal != signal_wide) return 1;
    if (selected_signal != signal_negative) return 2;
    if (calibration.scale != 7.5f || calibration.origin != 2.0 ||
        calibration.step != -7.5 || calibration.integral_bias != 10) return 3;
    if (calibration.sample.fraction != 0.125 || !calibration.quality.active ||
        calibration.quality.channel != 17 || calibration.quality.adjustment != -5) return 4;
    if (narrow_message[6] != 'A' || narrow_message[7] != 'B') return 5;
    if (wide_message[5] != 0x03a9 || wide_message[6] != 'S') return 6;
    if (continued_message[5] != 's' || escaped_character != 255) return 7;
    return 0;
}
