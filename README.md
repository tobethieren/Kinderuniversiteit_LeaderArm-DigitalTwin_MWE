# Kinderuniversiteit LeaderArm Digital Twin MWE

A minimal working example for driving a Blender robot arm digital twin from an Arduino-based leader arm.

This repository contains the most straightforward version of the setup:
- an **interactive calibration sketch** for Arduino,
- a **streaming sketch** that sends joint angles over Serial,
- a **Blender file** for the digital twin,
- and a **Blender Python script** that reads the serial data and applies it to the armature.

## Repository structure

```text
Kinderuniversiteit_LeaderArm-DigitalTwin_MWE/
├── arduino/
│   ├── LeaderArm_Calibration/
│   │   └── LeaderArm_Calibration.ino
│   └── LeaderArm_Blender/
│       └── LeaderArm_Blender.ino
├── blender/
│   ├── leader_arm_blender_serial.py
│   └── robot_arm.blend
├── .gitignore
├── LICENSE
└── README.md
```

## What to do first

**Always run the calibration sketch first.**

Do not start with the Blender streaming sketch before calibration, otherwise your angle mapping may be wrong.

## Quick start

### 1. Calibrate the leader arm in Arduino

1. Open `arduino/LeaderArm_Calibration/LeaderArm_Calibration.ino` in the Arduino IDE.
2. Upload it to your Arduino.
3. Open **Serial Monitor** with **Ctrl+Shift+M**.
4. Set the baud rate to **115200**.
5. Set line ending to **Both NL & CR**.
6. Follow the prompts for each joint.
7. At the end, the sketch prints a calibration block like this:

```cpp
const JointCalibration BASIS_CAL    = {...};
const JointCalibration MAIN_ARM_CAL = {...};
const JointCalibration FORE_ARM_CAL = {...};
const JointCalibration WRIST_CAL    = {...};
```

8. Copy that block.

Reference screenshot:
`assets/arduino-serial-monitor-settings.png`

### 2. Upload the Blender streaming sketch

1. Open `arduino/LeaderArm_Blender/LeaderArm_Blender.ino`.
2. Replace the example calibration values with the block you copied from the calibration sketch.
3. Upload the sketch to the Arduino.
4. Confirm that the Arduino is streaming CSV angle values.

The serial output format is:

```text
basis,main,fore,wrist
```

### 3. Close Serial Monitor before using Blender

Before starting Blender communication, **close Serial Monitor**.

This is important because the serial port can usually only be opened by one application at a time. If Serial Monitor is still open, Blender will not be able to connect to the Arduino.

## Blender setup

The Blender file can be downloaded directly from this repository:
- `blender/robot_arm.blend`

The Blender serial script is also included as a standalone file:
- `blender/leader_arm_blender_serial.py`

The project version used here is intended to work with the script already available from inside the Blender file as well, so you can either:
- open the `.blend` file and run the script from Blender's Text Editor, or
- load the standalone `leader_arm_blender_serial.py` manually.

### Steps in Blender

1. Open `blender/robot_arm.blend`.
2. Go to the **Scripting** workspace.
3. Open or verify `leader_arm_blender_serial.py` in Blender's scripting tab.
4. Click **Run Script**.
5. Move the physical leader arm.

If Blender selects the wrong serial port, set it manually near the top of `blender/leader_arm_blender_serial.py`:

```python
SERIAL_PORT = "COM3"
```

Replace `COM3` with the correct port on your system.

## Notes

- The Blender script expects an armature named `Armature`.
- The serial baud rate is **115200**.
- The update interval is set to **20 ms**.
- The base joint is configured for approximately **-150° to +150°**.

## Publishing this project

This repository includes an MIT license in the root folder.

```text
Copyright (c) 2026 Tobe Thieren - KU Leuven Gent
```

## License

This project is licensed under the MIT License. See `LICENSE` for details.
