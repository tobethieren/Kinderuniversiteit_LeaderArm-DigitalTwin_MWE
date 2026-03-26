import importlib
import math
import subprocess
import sys
import time

import bpy
from mathutils import Quaternion

# ======================================================
# Basic settings
# Set SERIAL_PORT to something like "COM3" if auto-detect
# picks the wrong port. Leave it as None for auto-detect.
# ======================================================
SERIAL_PORT = None
BAUDRATE = 115200
ARMATURE_NAME = "Armature"
UPDATE_INTERVAL_SECONDS = 0.02
AUTO_INSTALL_PYSERIAL = True
# ======================================================

STATE_KEY = "leader_arm_serial_listener_state"

BONES = {
    "basis": {
        "bone_name": "BONE_BASIS",
        "axis": "Y",
        "invert": False,
        "offset_deg": 0.0,
    },
    "main": {
        "bone_name": "BONE_MAIN_ARM",
        "axis": "X",
        "invert": True,
        "offset_deg": -90.0,
    },
    "fore": {
        "bone_name": "BONE_FORE_ARM",
        "axis": "X",
        "invert": False,
        "offset_deg": 90.0,
    },
    "wrist": {
        "bone_name": "BONE_WRIST",
        "axis": "X",
        "invert": True,
        "offset_deg": 0.0,
    },
}


def _unique(values):
    result = []
    for value in values:
        if value and value not in result:
            result.append(value)
    return result



def clamp(value, low, high):
    return max(low, min(high, value))



def ensure_serial_modules():
    try:
        serial = importlib.import_module("serial")
        list_ports = importlib.import_module("serial.tools.list_ports")
        return serial, list_ports
    except Exception as first_error:
        print(f"pyserial not available yet: {first_error}")

    if not AUTO_INSTALL_PYSERIAL:
        raise RuntimeError(
            "pyserial is not installed. Set AUTO_INSTALL_PYSERIAL to True or install pyserial manually."
        )

    python_candidates = _unique([
        getattr(bpy.app, "binary_path_python", None),
        sys.executable,
    ])

    last_error = None
    for python_exe in python_candidates:
        try:
            print(f"Trying to install pyserial via: {python_exe}")
            subprocess.run([python_exe, "-m", "ensurepip", "--upgrade"], check=False)
            subprocess.run([python_exe, "-m", "pip", "install", "--upgrade", "pip"], check=False)
            subprocess.run([python_exe, "-m", "pip", "install", "pyserial"], check=True)

            importlib.invalidate_caches()
            serial = importlib.import_module("serial")
            list_ports = importlib.import_module("serial.tools.list_ports")
            print("pyserial installed successfully.")
            return serial, list_ports
        except Exception as install_error:
            last_error = install_error
            print(f"Installation failed via {python_exe}: {install_error}")

    raise RuntimeError(
        "Could not install pyserial automatically.
"
        "Open Blender's Python console or terminal and install pyserial manually.
"
        f"Last error: {last_error}"
    )


SERIAL_MODULE, LIST_PORTS_MODULE = ensure_serial_modules()



def stop_previous_listener():
    state = bpy.app.driver_namespace.get(STATE_KEY)
    if not state:
        return

    timer_fn = state.get("timer_fn")
    ser = state.get("serial")

    try:
        if timer_fn and bpy.app.timers.is_registered(timer_fn):
            bpy.app.timers.unregister(timer_fn)
    except Exception:
        pass

    try:
        if ser and getattr(ser, "is_open", False):
            ser.close()
    except Exception:
        pass

    bpy.app.driver_namespace.pop(STATE_KEY, None)
    print("Stopped previous Blender serial listener.")



def choose_serial_port():
    available_ports = list(LIST_PORTS_MODULE.comports())

    if SERIAL_PORT:
        for port in available_ports:
            if port.device == SERIAL_PORT:
                print(f"Using serial port from script: {SERIAL_PORT}")
                return SERIAL_PORT
        raise RuntimeError(f"Configured serial port does not exist: {SERIAL_PORT}")

    if not available_ports:
        raise RuntimeError("No serial ports found. Connect the Arduino first.")

    preferred_ports = []
    for port in available_ports:
        description = (port.description or "").lower()
        manufacturer = (port.manufacturer or "").lower()
        if (
            "arduino" in description
            or "arduino" in manufacturer
            or "usb serial" in description
            or "ch340" in description
            or "cp210" in description
            or "nano" in description
            or "uno" in description
        ):
            preferred_ports.append(port)

    if len(preferred_ports) == 1:
        print(f"Automatically selected Arduino port: {preferred_ports[0].device}")
        return preferred_ports[0].device

    if len(available_ports) == 1:
        print(f"Automatically selected only available port: {available_ports[0].device}")
        return available_ports[0].device

    print("Multiple serial ports found:")
    for port in available_ports:
        print(f"- {port.device}: {port.description}")

    first_port = preferred_ports[0] if preferred_ports else available_ports[0]
    print(f"Selected first usable port: {first_port.device}")
    print("Set SERIAL_PORT manually at the top of the script if Blender picks the wrong one.")
    return first_port.device



def parse_csv_angle_line(line: str):
    parts = [part.strip() for part in line.split(",")]
    if len(parts) != 4:
        return None

    try:
        return tuple(float(part) for part in parts)
    except ValueError:
        return None



def apply_angle_to_bone(pose_bone, axis, angle_deg, invert=False, offset_deg=0.0):
    final_deg = (-angle_deg if invert else angle_deg) + offset_deg

    # Handle the base separately: quaternion rotation prevents
    # flips / jumps around 180 degrees.
    if pose_bone.name == BONES["basis"]["bone_name"]:
        final_deg = clamp(final_deg, -150.0, 150.0)
        angle_rad = math.radians(final_deg)

        if axis == "X":
            axis_vec = (1.0, 0.0, 0.0)
        elif axis == "Y":
            axis_vec = (0.0, 1.0, 0.0)
        elif axis == "Z":
            axis_vec = (0.0, 0.0, 1.0)
        else:
            raise ValueError(f"Unknown rotation axis: {axis}")

        pose_bone.rotation_mode = 'QUATERNION'
        pose_bone.rotation_quaternion = Quaternion(axis_vec, angle_rad)
        return

    pose_bone.rotation_mode = 'XYZ'
    angle_rad = math.radians(final_deg)

    if axis == "X":
        pose_bone.rotation_euler.x = angle_rad
    elif axis == "Y":
        pose_bone.rotation_euler.y = angle_rad
    elif axis == "Z":
        pose_bone.rotation_euler.z = angle_rad



def apply_angles(basis_deg, main_deg, fore_deg, wrist_deg):
    arm_obj = bpy.data.objects.get(ARMATURE_NAME)
    if arm_obj is None:
        print(f"Armature not found: {ARMATURE_NAME}")
        return

    if arm_obj.type != 'ARMATURE':
        print(f"Object '{ARMATURE_NAME}' is not an armature.")
        return

    values = {
        "basis": basis_deg,
        "main": main_deg,
        "fore": fore_deg,
        "wrist": wrist_deg,
    }

    for key, cfg in BONES.items():
        bone = arm_obj.pose.bones.get(cfg["bone_name"])
        if bone is None:
            print(f"Bone not found: {cfg['bone_name']}")
            continue

        apply_angle_to_bone(
            pose_bone=bone,
            axis=cfg["axis"],
            angle_deg=values[key],
            invert=cfg["invert"],
            offset_deg=cfg["offset_deg"],
        )

    bpy.context.view_layer.update()



def start_listener():
    stop_previous_listener()

    port_name = choose_serial_port()

    try:
        ser = SERIAL_MODULE.Serial(port_name, BAUDRATE, timeout=0.01)
    except Exception as exc:
        print(f"Could not open serial port: {port_name}")
        print(f"Error: {exc}")
        print("Close Arduino Serial Monitor / Plotter first.")
        return

    time.sleep(2.0)

    print("Blender serial listener started.")
    print(f"Port: {port_name} @ {BAUDRATE}")
    print(f"Armature: {ARMATURE_NAME}")
    print("Expected CSV: basis,main,fore,wrist")

    def timer_callback():
        try:
            for _ in range(20):
                raw = ser.readline()
                if not raw:
                    break

                line = raw.decode("utf-8", errors="ignore").strip()
                if not line:
                    continue

                angles = parse_csv_angle_line(line)
                if angles is None:
                    continue

                basis_deg, main_deg, fore_deg, wrist_deg = angles
                apply_angles(basis_deg, main_deg, fore_deg, wrist_deg)

        except Exception as exc:
            print(f"Error in Blender serial listener: {exc}")
            try:
                if ser.is_open:
                    ser.close()
            except Exception:
                pass
            bpy.app.driver_namespace.pop(STATE_KEY, None)
            return None

        return UPDATE_INTERVAL_SECONDS

    bpy.app.driver_namespace[STATE_KEY] = {
        "serial": ser,
        "timer_fn": timer_callback,
    }
    bpy.app.timers.register(timer_callback)


start_listener()
