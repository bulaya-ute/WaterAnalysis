import time


def interpolate_color(color1, color2, t):
    # Interpolate between two RGB colors
    return tuple(int((1 - t) * c1 + t * c2) for c1, c2 in zip(color1, color2))


def rgb_to_hex(rgb):
    return '#{:02x}{:02x}{:02x}'.format(*rgb)


def hex_to_rgb(hex_color):
    hex_color = hex_color.lstrip('#')
    return tuple(int(hex_color[i:i + 2], 16) for i in (0, 2, 4))


def darken_color(rgb, factor=0.3):
    # Darken the color by a given factor (e.g., 0.1 for 10% darker)
    return tuple(max(int(c * (1 - factor)), 0) for c in rgb)


def analyze_data(temperature, turbidity, safe_color=None, unsafe_color=None) -> dict:
    def scale(x):
        return ((1 / (1 + 2 ** (-x))) - 0.5) * 2

    upper_temp, lower_temp = 37, 20
    mid_temp = (upper_temp + lower_temp) / 2

    # Temperature safety rating
    temp_safety = scale(abs((temperature - mid_temp) / 4)) ** 3

    # Turbidity safety rating
    turbidity_safety = (1.0 - turbidity) ** 2

    # Final safety rating
    final_safety_rating = temp_safety * turbidity_safety

    # Determine description
    if final_safety_rating >= 0.8:
        description = "Safe to drink. Minimal threats."
    elif 0.6 <= final_safety_rating < 0.8:
        description = "Fairly safe to drink, but may still harbor pathogens."
    elif 0.4 <= final_safety_rating < 0.6:
        if temperature < 20 or temperature > 37:
            description = "May not harbor cholera due to the temperature, but isn't very clean."
        else:
            description = "Relatively clean, but the temperature is in a range that could support cholera."
    elif 0.2 <= final_safety_rating < 0.4:
        if turbidity > 0.5:
            description = "Water is very unclear and is likely unsafe."
        elif 0.2 < turbidity <= 0.5:
            description = "Water is slightly unclear and may be unsafe."
        else:
            description = "Looks clean, but might contain cholera."
    else:
        description = "Highly unsafe. Do not drink!"

    result = {
        "safety_rating": round(final_safety_rating, 3),
        "description": description
    }

    if safe_color and unsafe_color:
        if isinstance(safe_color, tuple) and isinstance(unsafe_color, tuple):
            output_hex = False
        else:
            output_hex = True

        # Convert hex colors to RGB if necessary
        if isinstance(safe_color, str):
            safe_color = hex_to_rgb(safe_color)
        if isinstance(unsafe_color, str):
            unsafe_color = hex_to_rgb(unsafe_color)

        # Interpolate colors based on safety rating
        interpolated_color = interpolate_color(unsafe_color, safe_color, final_safety_rating)
        darkened_color = darken_color(interpolated_color, 0.1)

        # Convert result back to hex if the inputs were hex strings
        if output_hex:
            result["color"] = rgb_to_hex(interpolated_color)
            result["color_alt"] = rgb_to_hex(darkened_color)
        else:
            result["color"] = interpolated_color
            result["color_alt"] = darkened_color

    return result


def get_sensor_status(timeout: float, last_online: float | None):
    now = time.time()
    if last_online is None:
        return {"status": "Sensor offline", "color": "#d3d3d3"}
    elif now - last_online < timeout:
        return {"status": "Sensor online", "color": "green"}
    return {"status": "Sensor offline", "color": "#d3d3d3"}
