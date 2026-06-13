def next_step_decis(current_decis, is_fstop, precision_value):
    if is_fstop:
        multiplier = 2.0 ** (1.0 / precision_value)
        new_target = int(current_decis * multiplier + 0.5)
        if new_target <= current_decis:
            new_target = current_decis + 1
    else:
        new_target = current_decis + precision_value * 10
    if new_target > 9999:
        new_target = 9999
    if new_target < 1:
        new_target = 1
    return new_target
