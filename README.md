# RS485 to Wiegand Bridge (on Dahua)

Necessity is the mother of invention. This project was born out of a simple need: DAHUA stopped providing firmware updates for older camera models, yet I urgently needed to integrate a **DHI-ITC215-PW4I-RIRLZF27135** camera into a modern access control system powered by a **Roger MC-16** controller.

## How It Works

The code intercepts data frames sent to the camera’s display unit (**IPMECS-2201C-IR**). The process is as follows:

1. **Capture**: Intercepts the display data frames.
2. **Hashing**: Performs **SHA1** hashing on the received array.
3. **Conversion**: Extracts the last 24 bits, calculates the checksum, and constructs a **26-bit WIEGAND** frame.
4. **Transmission**: Forwards the frame to a **Roger MCI-7** logic level converter to interface with the access controller.

## Hardware Setup

* **Microcontroller**: Arduino Pro Micro
* **Interface**: Generic MAX485 module

## Field Deployment

This solution is currently in active, daily operation at **ROD "Koninko"** in Świątniczki (near Poznań, Poland).

## License

This project is licensed under the **MIT License**. See the [LICENSE](LICENSE) file for details.
