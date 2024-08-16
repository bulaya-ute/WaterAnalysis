def water_safety_analysis(temperature, turbidity):
    # Temperature safety rating
    if 20 <= temperature <= 37:
        temp_safety = 1.0 - (abs(28.5 - temperature) / 18.5)  # Centered at 28.5
    else:
        temp_safety = max(0.0, 1.0 - (abs(28.5 - temperature) / 18.5))

    # Turbidity safety rating
    turbidity_safety = 1.0 - turbidity

    # Final safety rating
    final_safety_rating = temp_safety * turbidity_safety

    # Determine description
    if final_safety_rating >= 0.8:
        description = "The water is very clean and safe. It poses minimal risks."
    elif 0.6 <= final_safety_rating < 0.8:
        description = "The water is fairly safe but exercise caution. It may still harbor some pathogens."
    elif 0.4 <= final_safety_rating < 0.6:
        if temperature < 20 or temperature > 37:
            description = "The water may not harbor cholera due to the temperature, but it isn't very clean."
        else:
            description = "The water is relatively clean, but the temperature is in a range that could support cholera."
    elif 0.2 <= final_safety_rating < 0.4:
        if turbidity > 0.5:
            description = ("The water is quite turbid and may be unsafe. It might not have cholera, but other "
                           "contaminants are possible.")
        else:
            description = "The water is somewhat clean, but its temperature is likely to support cholera."
    else:
        description = "The water is highly unsafe. It is either too dirty or at a dangerous temperature, or both."

    # Convert safety rating to a percentage
    safety_percentage = round(final_safety_rating * 100, 2)

    return {"safety_rating": safety_percentage, "decription": description}
