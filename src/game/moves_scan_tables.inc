/* Recovered from the retail .data symbols and relocations.
 *
 * The retail command ABI interleaves 32-bit opcodes and function addresses.
 * MWCC's C90 mode cannot select a non-first union member in an initializer,
 * so relocation words use the one explicit function-to-word conversion at
 * this serialization boundary. No compiler extension or intrinsic is used.
 */
typedef union MovesScanWord {
    unsigned int value;
    MovesEntryFn action;
    void (*void_action)(void);
} MovesScanWord;

MovesScanWord jump_table[102] = {
    {0x00000001u},
    {(unsigned int)x_block},
    {0x00000001u},
    {(unsigned int)rotate_toward_j_exit},
    {0x00000001u},
    {(unsigned int)step_backward},
    {0x00000001u},
    {(unsigned int)step_forward},
    {0x00000001u},
    {(unsigned int)walk_backward},
    {0x00000001u},
    {(unsigned int)walk_forward},
    {0x00000001u},
    {(unsigned int)blend_to_stance_j_exit},
    {0x00000001u},
    {(unsigned int)blend_to_fstance_j_exit},
    {0x00000001u},
    {(unsigned int)joy_dash_back},
    {0x00000001u},
    {(unsigned int)weapon_block},
    {0x00000001u},
    {(unsigned int)block_a_intro},
    {0x00000001u},
    {(unsigned int)block_a_intro_glitch},
    {0x00000001u},
    {(unsigned int)block_a},
    {0x00000001u},
    {(unsigned int)block_b},
    {0x00000001u},
    {(unsigned int)block_c},
    {0x00000001u},
    {(unsigned int)block_d},
    {0x00000001u},
    {(unsigned int)step_left},
    {0x00000001u},
    {(unsigned int)walk_left},
    {0x00000001u},
    {(unsigned int)step_right},
    {0x00000001u},
    {(unsigned int)walk_right},
    {0x00000001u},
    {(unsigned int)joy_duck_remote_start},
    {0x00000001u},
    {(unsigned int)joy_duck_remote_end},
    {0x00000001u},
    {(unsigned int)j_flying_kick},
    {0x00000001u},
    {(unsigned int)j_flying_kick1_early},
    {0x00000001u},
    {(unsigned int)j_flying_kick2},
    {0x00000001u},
    {(unsigned int)j_flying_kick2_early},
    {0x00000001u},
    {(unsigned int)j_flying_punch},
    {0x00000001u},
    {(unsigned int)j_flying_punch_early},
    {0x00000001u},
    {(unsigned int)jump_away_opponent_j_exit},
    {0x00000001u},
    {(unsigned int)jump_towards_opponent_j_exit},
    {0x00000001u},
    {(unsigned int)blend_to_duck_block},
    {0x00000001u},
    {(unsigned int)r_hit_wall},
    {0x00000001u},
    {(unsigned int)throw_spear},
    {0x00000001u},
    {(unsigned int)tug_in_spear},
    {0x00000001u},
    {(unsigned int)retract_spear},
    {0x00000001u},
    {(unsigned int)go_into_twitch_death},
    {0x00000001u},
    {(unsigned int)go_into_major_pain},
    {0x00000001u},
    {(unsigned int)j_ass_rollup},
    {0x00000001u},
    {(unsigned int)back_rollup_left},
    {0x00000001u},
    {(unsigned int)back_rollup_right},
    {0x00000001u},
    {(unsigned int)front_rollup},
    {0x00000001u},
    {(unsigned int)front_rollup_left},
    {0x00000001u},
    {(unsigned int)front_rollup_right},
    {0x00000001u},
    {(unsigned int)jump_landing_j_exit},
    {0x00000001u},
    {(unsigned int)dizzy},
    {0x00000001u},
    {(unsigned int)r_chest2_stumble},
    {0x00000001u},
    {(unsigned int)do_my_fatality_remote},
    {0x00000001u},
    {(unsigned int)x_attack_5_remote},
    {0x00000001u},
    {(unsigned int)do_my_suicide_remote},
    {0x00000001u},
    {(unsigned int)do_my_2nd_fatality_remote},
    {0x00000001u},
    {(unsigned int)back_to_crouch},
};

MovesScanWord scan_nightwolf_suicide[8] = {
    {0x3FFFFFFDu},
    {0x01000032u},
    {(unsigned int)do_my_suicide},
    {0x80000003u},
    {0x80000003u},
    {0x80000003u},
    {0x80000003u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_boraicho_suicide[8] = {
    {0x3FFFFFFDu},
    {0x01000032u},
    {(unsigned int)do_my_suicide},
    {0x80000002u},
    {0x80000002u},
    {0x80000001u},
    {0x80000001u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_scorpion_suicide[8] = {
    {0x3FFFFFFDu},
    {0x01000032u},
    {(unsigned int)do_my_suicide},
    {0x80000004u},
    {0x80000004u},
    {0x80000003u},
    {0x80000002u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_baraka_suicide[8] = {
    {0x3FFFFFFDu},
    {0x01000032u},
    {(unsigned int)do_my_suicide},
    {0x80000004u},
    {0x80000002u},
    {0x80000003u},
    {0x80000002u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_subzero_suicide[8] = {
    {0x3FFFFFFDu},
    {0x01000032u},
    {(unsigned int)do_my_suicide},
    {0x80000004u},
    {0x80000003u},
    {0x80000004u},
    {0x80000003u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_mileena_suicide[8] = {
    {0x3FFFFFFDu},
    {0x01000032u},
    {(unsigned int)do_my_suicide},
    {0x80000002u},
    {0x80000004u},
    {0x80000001u},
    {0x80000002u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_ermac_suicide[8] = {
    {0x3FFFFFFDu},
    {0x01000032u},
    {(unsigned int)do_my_suicide},
    {0x80000004u},
    {0x80000003u},
    {0x80000003u},
    {0x80000004u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_ashrah_suicide[8] = {
    {0x3FFFFFFDu},
    {0x01000032u},
    {(unsigned int)do_my_suicide},
    {0x80000003u},
    {0x80000004u},
    {0x80000003u},
    {0x80000003u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_sindel_suicide[8] = {
    {0x3FFFFFFDu},
    {0x01000032u},
    {(unsigned int)do_my_suicide},
    {0x80000001u},
    {0x80000003u},
    {0x80000003u},
    {0x80000004u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_limei_suicide[8] = {
    {0x3FFFFFFDu},
    {0x01000032u},
    {(unsigned int)do_my_suicide},
    {0x80000003u},
    {0x80000004u},
    {0x80000003u},
    {0x80000004u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_hotaru_suicide[8] = {
    {0x3FFFFFFDu},
    {0x01000032u},
    {(unsigned int)do_my_suicide},
    {0x80000003u},
    {0x80000002u},
    {0x80000002u},
    {0x80000002u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_kenshi_suicide[8] = {
    {0x3FFFFFFDu},
    {0x01000032u},
    {(unsigned int)do_my_suicide},
    {0x80000004u},
    {0x80000002u},
    {0x80000002u},
    {0x80000001u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_skab_suicide[8] = {
    {0x3FFFFFFDu},
    {0x01000032u},
    {(unsigned int)do_my_suicide},
    {0x80000001u},
    {0x80000003u},
    {0x80000003u},
    {0x80000003u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_tanya_suicide[8] = {
    {0x3FFFFFFDu},
    {0x01000032u},
    {(unsigned int)do_my_suicide},
    {0x80000004u},
    {0x80000004u},
    {0x80000001u},
    {0x80000003u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_liukang_suicide[8] = {
    {0x3FFFFFFDu},
    {0x01000032u},
    {(unsigned int)do_my_suicide},
    {0x80000004u},
    {0x80000004u},
    {0x80000004u},
    {0x80000003u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_kira_suicide[8] = {
    {0x3FFFFFFDu},
    {0x01000032u},
    {(unsigned int)do_my_suicide},
    {0x80000001u},
    {0x80000002u},
    {0x80000003u},
    {0x80000002u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_kabal_suicide[8] = {
    {0x3FFFFFFDu},
    {0x01000032u},
    {(unsigned int)do_my_suicide},
    {0x80000001u},
    {0x80000003u},
    {0x80000003u},
    {0x80000004u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_kobra_suicide[7] = {
    {0x3FFFFFFDu},
    {0x01000032u},
    {(unsigned int)do_my_suicide},
    {0x80000003u},
    {0x80000002u},
    {0x80000002u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_jade_suicide[8] = {
    {0x3FFFFFFDu},
    {0x01000032u},
    {(unsigned int)do_my_suicide},
    {0x80000001u},
    {0x80000001u},
    {0x80000001u},
    {0x80000002u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_dairou_suicide[8] = {
    {0x3FFFFFFDu},
    {0x01000032u},
    {(unsigned int)do_my_suicide},
    {0x80000002u},
    {0x80000002u},
    {0x80000002u},
    {0x80000004u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_raiden_suicide[8] = {
    {0x3FFFFFFDu},
    {0x01000032u},
    {(unsigned int)do_my_suicide},
    {0x80000001u},
    {0x80000003u},
    {0x80000003u},
    {0x80000002u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_cassius_suicide[7] = {
    {0x3FFFFFFDu},
    {0x01000032u},
    {(unsigned int)do_my_suicide},
    {0x80000002u},
    {0x80000001u},
    {0x80000001u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_shujinko_suicide[8] = {
    {0x3FFFFFFDu},
    {0x01000032u},
    {(unsigned int)do_my_suicide},
    {0x80000004u},
    {0x80000002u},
    {0x80000004u},
    {0x80000002u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_noobsmoke_suicide[8] = {
    {0x3FFFFFFDu},
    {0x01000032u},
    {(unsigned int)do_my_suicide},
    {0x80000004u},
    {0x80000003u},
    {0x80000003u},
    {0x80000004u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_goro_suicide[8] = {
    {0x3FFFFFFDu},
    {0x01000032u},
    {(unsigned int)do_my_suicide},
    {0x80000003u},
    {0x80000001u},
    {0x80000001u},
    {0x80000002u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_shao_kahn_suicide[8] = {
    {0x3FFFFFFDu},
    {0x01000032u},
    {(unsigned int)do_my_suicide},
    {0x80000004u},
    {0x80000003u},
    {0x80000003u},
    {0x80000004u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_boraicho_1[6] = {
    {0x00000000u},
    {0x03000019u},
    {0x00000010u},
    {0x80000004u},
    {0x80000002u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_boraicho_2[13] = {
    {0x00004205u},
    {0x03000019u},
    {0x0000000Au},
    {0x80000002u},
    {0x80000001u},
    {0x3FFFFFFEu},
    {0x01000032u},
    {(unsigned int)do_my_fatality},
    {0x80000004u},
    {0x80000003u},
    {0x80000001u},
    {0x80000001u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_boraicho_3[12] = {
    {0x3FFFFFFEu},
    {0x01000032u},
    {(unsigned int)do_my_2nd_fatality},
    {0x80000003u},
    {0x80000002u},
    {0x80000001u},
    {0x00000000u},
    {0x03000019u},
    {0x0000000Eu},
    {0x80000004u},
    {0x80000003u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_boraicho_4[6] = {
    {0x00000000u},
    {0x03000019u},
    {0x0000000Bu},
    {0x80000001u},
    {0x80000001u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_limei_1[13] = {
    {0x0000424Fu},
    {0x03000019u},
    {0x0000000Du},
    {0x80000004u},
    {0x80000002u},
    {0x3FFFFFFEu},
    {0x01000032u},
    {(unsigned int)do_my_fatality},
    {0x80000001u},
    {0x80000001u},
    {0x80000001u},
    {0x80000001u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_limei_2[6] = {
    {0x00000000u},
    {0x03000019u},
    {0x00000010u},
    {0x80000001u},
    {0x80000001u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_limei_3[6] = {
    {0x00000000u},
    {0x03000019u},
    {0x0000000Fu},
    {0x80000001u},
    {0x80000004u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_limei_4[13] = {
    {0x00000000u},
    {0x03000019u},
    {0x00000012u},
    {0x80000004u},
    {0x80000002u},
    {0x3FFFFFFEu},
    {0x01000032u},
    {(unsigned int)do_my_2nd_fatality},
    {0x80000003u},
    {0x80000002u},
    {0x80000001u},
    {0x80000001u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_hotaru_1[13] = {
    {0x00004215u},
    {0x03000019u},
    {0x0000000Cu},
    {0x80000004u},
    {0x80000002u},
    {0x3FFFFFFEu},
    {0x01000032u},
    {(unsigned int)do_my_fatality},
    {0x80000001u},
    {0x80000003u},
    {0x80000002u},
    {0x80000004u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_hotaru_2[8] = {
    {0x3FFFFFFEu},
    {0x01000032u},
    {(unsigned int)do_my_2nd_fatality},
    {0x80000004u},
    {0x80000001u},
    {0x80000002u},
    {0x80000001u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_hotaru_4[6] = {
    {0x00000000u},
    {0x03000019u},
    {0x00000007u},
    {0x80000001u},
    {0x80000001u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_nightwolf_1[18] = {
    {0x00004251u},
    {0x03000019u},
    {0x00000010u},
    {0x80000004u},
    {0x80000002u},
    {0x00004216u},
    {0x03000019u},
    {0x00000016u},
    {0x80000004u},
    {0x80000003u},
    {0x3FFFFFFEu},
    {0x01000032u},
    {(unsigned int)do_my_fatality},
    {0x80000002u},
    {0x80000001u},
    {0x80000002u},
    {0x80000001u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_nightwolf_2[13] = {
    {0x3FFFFFFEu},
    {0x01000032u},
    {(unsigned int)do_my_2nd_fatality},
    {0x80000004u},
    {0x80000001u},
    {0x80000004u},
    {0x80000003u},
    {0x00000000u},
    {0x03000019u},
    {0x00000014u},
    {0x80000001u},
    {0x80000002u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_nightwolf_4[6] = {
    {0x00004246u},
    {0x03000019u},
    {0x00000012u},
    {0x80000001u},
    {0x80000001u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_ermac_1[6] = {
    {0x0000424Du},
    {0x03000019u},
    {0x00000012u},
    {0x80000004u},
    {0x80000002u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_ermac_2[6] = {
    {0x00000000u},
    {0x0300000Fu},
    {0x00000019u},
    {0x80000001u},
    {0x80000002u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_ermac_3[13] = {
    {0x00000000u},
    {0x03000028u},
    {0x0000001Eu},
    {0x80000004u},
    {0x80000003u},
    {0x3FFFFFFEu},
    {0x01000032u},
    {(unsigned int)do_my_fatality},
    {0x80000004u},
    {0x80000002u},
    {0x80000002u},
    {0x80000004u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_ermac_4[14] = {
    {0x00004219u},
    {0x03000028u},
    {0x00000017u},
    {0x80000002u},
    {0x80000004u},
    {0x80000002u},
    {0x3FFFFFFEu},
    {0x01000032u},
    {(unsigned int)do_my_2nd_fatality},
    {0x80000002u},
    {0x80000004u},
    {0x80000002u},
    {0x80000004u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_mileena_1[8] = {
    {0x3FFFFFFEu},
    {0x01000032u},
    {(unsigned int)do_my_fatality},
    {0x80000001u},
    {0x80000001u},
    {0x80000004u},
    {0x80000004u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_mileena_2[6] = {
    {0x00004240u},
    {0x03000019u},
    {0x00000016u},
    {0x80000004u},
    {0x80000001u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_mileena_3[8] = {
    {0x3FFFFFFEu},
    {0x01000032u},
    {(unsigned int)do_my_2nd_fatality},
    {0x80000003u},
    {0x80000003u},
    {0x80000001u},
    {0x80000001u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_mileena_4[11] = {
    {0x00000000u},
    {0x03000019u},
    {0x0000000Bu},
    {0x80000001u},
    {0x80000001u},
    {0x00004214u},
    {0x03000019u},
    {0x0000000Eu},
    {0x80000002u},
    {0x80000004u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_ashrah_1[11] = {
    {0x00000000u},
    {0x03000019u},
    {0x00000010u},
    {0x80000004u},
    {0x80000002u},
    {0x0000424Au},
    {0x03000019u},
    {0x00000014u},
    {0x80000004u},
    {0x80000001u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_ashrah_2[15] = {
    {0x3FFFFFFEu},
    {0x01000032u},
    {(unsigned int)do_my_2nd_fatality},
    {0x80000001u},
    {0x80000004u},
    {0x80000001u},
    {0x80000004u},
    {0x3FFFFFFEu},
    {0x01000032u},
    {(unsigned int)do_my_fatality},
    {0x80000004u},
    {0x80000004u},
    {0x80000003u},
    {0x80000003u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_ashrah_3[6] = {
    {0x00000000u},
    {0x03000019u},
    {0x0000001Au},
    {0x80000004u},
    {0x80000003u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_ashrah_4[6] = {
    {0x00000000u},
    {0x03000019u},
    {0x0000001Cu},
    {0x80000001u},
    {0x80000001u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_sindel_1[18] = {
    {0x00004217u},
    {0x03000019u},
    {0x00000010u},
    {0x80000001u},
    {0x80000001u},
    {0x0000421Bu},
    {0x03000019u},
    {0x00000014u},
    {0x80000002u},
    {0x80000001u},
    {0x3FFFFFFEu},
    {0x01000032u},
    {(unsigned int)do_my_fatality},
    {0x80000002u},
    {0x80000001u},
    {0x80000001u},
    {0x80000002u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_sindel_3[13] = {
    {0x00004218u},
    {0x03000019u},
    {0x0000000Fu},
    {0x80000001u},
    {0x80000001u},
    {0x3FFFFFFEu},
    {0x01000032u},
    {(unsigned int)do_my_2nd_fatality},
    {0x80000003u},
    {0x80000003u},
    {0x80000002u},
    {0x80000001u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_baraka_1[6] = {
    {0x0000424Bu},
    {0x03000019u},
    {0x0000000Cu},
    {0x80000004u},
    {0x80000002u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_baraka_2[14] = {
    {0x3FFFFFFEu},
    {0x01000032u},
    {(unsigned int)do_my_fatality},
    {0x80000001u},
    {0x80000003u},
    {0x80000004u},
    {0x3FFFFFFEu},
    {0x01000032u},
    {(unsigned int)do_my_2nd_fatality},
    {0x80000001u},
    {0x80000004u},
    {0x80000002u},
    {0x80000001u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_baraka_3[11] = {
    {0x00004252u},
    {0x03000019u},
    {0x00000011u},
    {0x80000004u},
    {0x80000003u},
    {0x00000000u},
    {0x03000019u},
    {0x0000000Fu},
    {0x80000001u},
    {0x80000001u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_kenshi_1[7] = {
    {0x00004219u},
    {0x03000019u},
    {0x00000010u},
    {0x80000002u},
    {0x80000004u},
    {0x80000002u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_kenshi_2[20] = {
    {0x00000000u},
    {0x03000019u},
    {0x00000013u},
    {0x80000001u},
    {0x80000001u},
    {0x3FFFFFFEu},
    {0x01000032u},
    {(unsigned int)do_my_2nd_fatality},
    {0x80000001u},
    {0x80000001u},
    {0x80000002u},
    {0x80000002u},
    {0x3FFFFFFEu},
    {0x01000032u},
    {(unsigned int)do_my_fatality},
    {0x80000003u},
    {0x80000001u},
    {0x80000002u},
    {0x80000001u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_kenshi_3[11] = {
    {0x0000422Du},
    {0x03000019u},
    {0x0000000Eu},
    {0x80000004u},
    {0x80000002u},
    {0x0000423Au},
    {0x03000019u},
    {0x00000019u},
    {0x80000002u},
    {0x80000001u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_kenshi_4[6] = {
    {0x00000000u},
    {0x03000019u},
    {0x00000015u},
    {0x80000004u},
    {0x80000002u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_subzero_1[13] = {
    {0x00004212u},
    {0x03000019u},
    {0x0000000Au},
    {0x80000004u},
    {0x80000002u},
    {0x3FFFFFFEu},
    {0x01000032u},
    {(unsigned int)do_my_2nd_fatality},
    {0x80000002u},
    {0x80000004u},
    {0x80000001u},
    {0x80000004u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_subzero_2[8] = {
    {0x3FFFFFFEu},
    {0x01000032u},
    {(unsigned int)do_my_fatality},
    {0x80000001u},
    {0x80000002u},
    {0x80000004u},
    {0x80000001u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_subzero_3[6] = {
    {0x00001205u},
    {0x03000019u},
    {0x00000011u},
    {0x80000004u},
    {0x80000001u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_subzero_4[6] = {
    {0x00001214u},
    {0x0300001Eu},
    {0x00000017u},
    {0x80000002u},
    {0x80000001u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_jade_1[13] = {
    {0x00004228u},
    {0x03000019u},
    {0x0000001Eu},
    {0x80000002u},
    {0x80000001u},
    {0x3FFFFFFEu},
    {0x01000032u},
    {(unsigned int)do_my_fatality},
    {0x80000002u},
    {0x80000001u},
    {0x80000003u},
    {0x80000001u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_jade_2[13] = {
    {0x0000422Fu},
    {0x03000019u},
    {0x00000024u},
    {0x80000002u},
    {0x80000001u},
    {0x3FFFFFFEu},
    {0x01000032u},
    {(unsigned int)do_my_2nd_fatality},
    {0x80000002u},
    {0x80000001u},
    {0x80000001u},
    {0x80000001u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_jade_3[6] = {
    {0x0000421Du},
    {0x03000019u},
    {0x00000014u},
    {0x80000004u},
    {0x80000002u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_jade_4[6] = {
    {0x00004227u},
    {0x03000019u},
    {0x0000001Au},
    {0x80000002u},
    {0x80000001u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_scorpion_1[20] = {
    {0x00001203u},
    {0x0300001Bu},
    {0x00000007u},
    {0x80000002u},
    {0x80000001u},
    {0x3FFFFFFEu},
    {0x01000032u},
    {(unsigned int)do_my_fatality},
    {0x80000001u},
    {0x80000004u},
    {0x80000001u},
    {0x80000001u},
    {0x3FFFFFFEu},
    {0x01000032u},
    {(unsigned int)do_my_2nd_fatality},
    {0x80000001u},
    {0x80000002u},
    {0x80000001u},
    {0x80000002u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_scorpion_2[6] = {
    {0x0000420Bu},
    {0x03000019u},
    {0x00000009u},
    {0x80000004u},
    {0x80000002u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_scorpion_3[11] = {
    {0x00000000u},
    {0x03000019u},
    {0x00000006u},
    {0x80000001u},
    {0x80000002u},
    {0x00004213u},
    {0x03000019u},
    {0x0000000Bu},
    {0x80000004u},
    {0x80000002u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_freak_1[6] = {
    {0x00001203u},
    {0x0300001Bu},
    {0x00000009u},
    {0x80000002u},
    {0x80000001u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_freak_2[6] = {
    {0x0000420Bu},
    {0x03000019u},
    {0x0000000Bu},
    {0x80000004u},
    {0x80000002u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_freak_3[11] = {
    {0x00000000u},
    {0x03000019u},
    {0x00000008u},
    {0x80000001u},
    {0x80000002u},
    {0x00004213u},
    {0x03000019u},
    {0x0000000Du},
    {0x80000004u},
    {0x80000002u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_kunglao_4[6] = {
    {0x00000000u},
    {0x03000019u},
    {0x00000005u},
    {0x80000004u},
    {0x80000002u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_kabal_1[6] = {
    {0x0000424Eu},
    {0x03000019u},
    {0x00000012u},
    {0x80000004u},
    {0x80000001u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_kabal_2[13] = {
    {0x00000000u},
    {0x03000019u},
    {0x00000013u},
    {0x80000004u},
    {0x80000002u},
    {0x3FFFFFFEu},
    {0x01000032u},
    {(unsigned int)do_my_2nd_fatality},
    {0x80000003u},
    {0x80000003u},
    {0x80000004u},
    {0x80000004u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_kabal_3[8] = {
    {0x3FFFFFFEu},
    {0x01000032u},
    {(unsigned int)do_my_fatality},
    {0x80000001u},
    {0x80000003u},
    {0x80000003u},
    {0x80000003u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_kabal_4[6] = {
    {0x00004249u},
    {0x03000019u},
    {0x0000001Au},
    {0x80000002u},
    {0x80000001u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_noob_1[6] = {
    {0x00001223u},
    {0x03000019u},
    {0x00000020u},
    {0x80000003u},
    {0x80000004u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_smoke_1[6] = {
    {0x00001223u},
    {0x03000019u},
    {0x0000001Fu},
    {0x80000003u},
    {0x80000004u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_noob_2[13] = {
    {0x00004248u},
    {0x03000019u},
    {0x00000030u},
    {0x80000004u},
    {0x80000002u},
    {0x3FFFFFFEu},
    {0x01000032u},
    {(unsigned int)do_my_2nd_fatality},
    {0x80000003u},
    {0x80000004u},
    {0x80000004u},
    {0x80000001u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_smoke_2[13] = {
    {0x00004248u},
    {0x03000019u},
    {0x0000003Fu},
    {0x80000004u},
    {0x80000002u},
    {0x3FFFFFFEu},
    {0x01000032u},
    {(unsigned int)do_my_2nd_fatality},
    {0x80000003u},
    {0x80000004u},
    {0x80000004u},
    {0x80000001u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_noobsmoke_3[11] = {
    {0x00000000u},
    {0x03000019u},
    {0x0000002Cu},
    {0x80000001u},
    {0x80000001u},
    {0x00000000u},
    {0x03000019u},
    {0x0000002Eu},
    {0x80000004u},
    {0x80000002u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_smoke_4[13] = {
    {0x0000421Fu},
    {0x03000019u},
    {0x0000001Cu},
    {0x80000001u},
    {0x80000001u},
    {0x3FFFFFFEu},
    {0x01000032u},
    {(unsigned int)do_my_fatality},
    {0x80000002u},
    {0x80000001u},
    {0x80000002u},
    {0x80000001u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_noob_4[8] = {
    {0x3FFFFFFEu},
    {0x01000032u},
    {(unsigned int)do_my_fatality},
    {0x80000002u},
    {0x80000001u},
    {0x80000002u},
    {0x80000001u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_dairou_1[14] = {
    {0x0000422Au},
    {0x03000023u},
    {0x0000000Bu},
    {0x80000001u},
    {0x80000002u},
    {0x80000004u},
    {0x3FFFFFFEu},
    {0x01000032u},
    {(unsigned int)do_my_fatality},
    {0x80000004u},
    {0x80000003u},
    {0x80000002u},
    {0x80000002u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_dairou_2[6] = {
    {0x0000424Cu},
    {0x03000019u},
    {0x00000011u},
    {0x80000004u},
    {0x80000002u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_dairou_3[12] = {
    {0x0000422Bu},
    {0x0300001Eu},
    {0x0000000Du},
    {0x80000004u},
    {0x80000003u},
    {0x3FFFFFFEu},
    {0x01000032u},
    {(unsigned int)do_my_2nd_fatality},
    {0x80000004u},
    {0x80000004u},
    {0x80000002u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_kobra_1[11] = {
    {0x00004247u},
    {0x03000019u},
    {0x00000010u},
    {0x80000004u},
    {0x80000001u},
    {0x00004247u},
    {0x03000019u},
    {0x0000001Bu},
    {0x80000004u},
    {0x80000002u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_kobra_2[18] = {
    {0x00000000u},
    {0x03000019u},
    {0x00000013u},
    {0x80000004u},
    {0x80000001u},
    {0x00000000u},
    {0x03000019u},
    {0x00000017u},
    {0x80000004u},
    {0x80000002u},
    {0x3FFFFFFEu},
    {0x01000032u},
    {(unsigned int)do_my_fatality},
    {0x80000001u},
    {0x80000002u},
    {0x80000001u},
    {0x80000001u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_kobra_4[13] = {
    {0x00000000u},
    {0x03000019u},
    {0x00000015u},
    {0x80000004u},
    {0x80000002u},
    {0x3FFFFFFEu},
    {0x01000032u},
    {(unsigned int)do_my_2nd_fatality},
    {0x80000004u},
    {0x80000002u},
    {0x80000001u},
    {0x80000004u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_tanya_1[13] = {
    {0x0000422Cu},
    {0x03000019u},
    {0x0000001Eu},
    {0x80000003u},
    {0x80000004u},
    {0x3FFFFFFEu},
    {0x01000032u},
    {(unsigned int)do_my_fatality},
    {0x80000001u},
    {0x80000004u},
    {0x80000004u},
    {0x80000004u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_tanya_2[18] = {
    {0x00004221u},
    {0x03000019u},
    {0x00000017u},
    {0x80000004u},
    {0x80000002u},
    {0x00004222u},
    {0x03000019u},
    {0x0000001Au},
    {0x80000004u},
    {0x80000003u},
    {0x3FFFFFFEu},
    {0x01000032u},
    {(unsigned int)do_my_2nd_fatality},
    {0x80000003u},
    {0x80000002u},
    {0x80000001u},
    {0x80000003u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_tanya_3[6] = {
    {0x00004220u},
    {0x03000019u},
    {0x00000014u},
    {0x80000001u},
    {0x80000001u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_tanya_4[6] = {
    {0x00004226u},
    {0x03000019u},
    {0x0000001Bu},
    {0x80000004u},
    {0x80000002u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_liukang_1[6] = {
    {0x00004250u},
    {0x03000019u},
    {0x00000014u},
    {0x80000001u},
    {0x80000001u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_liukang_2[13] = {
    {0x00004250u},
    {0x03000019u},
    {0x00000010u},
    {0x80000001u},
    {0x80000001u},
    {0x3FFFFFFEu},
    {0x01000032u},
    {(unsigned int)do_my_fatality},
    {0x80000002u},
    {0x80000002u},
    {0x80000002u},
    {0x80000001u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_liukang_3[13] = {
    {0x00004239u},
    {0x03000019u},
    {0x00000018u},
    {0x80000004u},
    {0x80000001u},
    {0x3FFFFFFEu},
    {0x01000032u},
    {(unsigned int)do_my_2nd_fatality},
    {0x80000001u},
    {0x80000001u},
    {0x80000003u},
    {0x80000003u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_liukang_4[6] = {
    {0x00003201u},
    {0x03000014u},
    {0x00000019u},
    {0x80000001u},
    {0x80000001u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_raiden_1[20] = {
    {0x0000120Eu},
    {0x03000019u},
    {0x00000013u},
    {0x80000004u},
    {0x80000002u},
    {0x3FFFFFFEu},
    {0x01000032u},
    {(unsigned int)do_my_fatality},
    {0x80000002u},
    {0x80000004u},
    {0x80000001u},
    {0x80000004u},
    {0x3FFFFFFEu},
    {0x01000032u},
    {(unsigned int)do_my_2nd_fatality},
    {0x80000003u},
    {0x80000004u},
    {0x80000001u},
    {0x80000001u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_raiden_2[6] = {
    {0x00000000u},
    {0x03000019u},
    {0x00000016u},
    {0x80000001u},
    {0x80000001u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_raiden_3[6] = {
    {0x00004229u},
    {0x03000019u},
    {0x00000022u},
    {0x80000004u},
    {0x80000003u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_raiden_4[6] = {
    {0x0000421Eu},
    {0x03000019u},
    {0x0000001Eu},
    {0x80000001u},
    {0x80000001u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_kira_1[6] = {
    {0x00004225u},
    {0x03000019u},
    {0x00000014u},
    {0x80000002u},
    {0x80000001u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_kira_3[13] = {
    {0x0000421Cu},
    {0x03000019u},
    {0x00000011u},
    {0x80000004u},
    {0x80000001u},
    {0x3FFFFFFEu},
    {0x01000032u},
    {(unsigned int)do_my_2nd_fatality},
    {0x80000003u},
    {0x80000001u},
    {0x80000004u},
    {0x80000002u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_kira_4[13] = {
    {0x00000000u},
    {0x03000019u},
    {0x00000017u},
    {0x80000002u},
    {0x80000001u},
    {0x3FFFFFFEu},
    {0x01000032u},
    {(unsigned int)do_my_fatality},
    {0x80000002u},
    {0x80000001u},
    {0x80000001u},
    {0x80000002u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_skab_1[11] = {
    {0x00004234u},
    {0x03000019u},
    {0x00000018u},
    {0x80000004u},
    {0x80000002u},
    {0x00004231u},
    {0x03000019u},
    {0x00000016u},
    {0x80000002u},
    {0x80000004u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_skab_2[13] = {
    {0x00004232u},
    {0x03000023u},
    {0x00000012u},
    {0x80000002u},
    {0x80000001u},
    {0x3FFFFFFEu},
    {0x01000032u},
    {(unsigned int)do_my_2nd_fatality},
    {0x80000001u},
    {0x80000001u},
    {0x80000001u},
    {0x80000002u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_skab_3[6] = {
    {0x00004230u},
    {0x03000019u},
    {0x00000011u},
    {0x80000002u},
    {0x80000001u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_skab_4[13] = {
    {0x00004233u},
    {0x03000019u},
    {0x00000013u},
    {0x80000001u},
    {0x80000002u},
    {0x3FFFFFFEu},
    {0x01000032u},
    {(unsigned int)do_my_fatality},
    {0x80000004u},
    {0x80000001u},
    {0x80000001u},
    {0x80000003u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_cassius_1[13] = {
    {0x00004238u},
    {0x03000019u},
    {0x00000017u},
    {0x80000002u},
    {0x80000001u},
    {0x3FFFFFFEu},
    {0x01000032u},
    {(unsigned int)do_my_2nd_fatality},
    {0x80000004u},
    {0x80000001u},
    {0x80000001u},
    {0x80000003u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_cassius_2[6] = {
    {0x00004236u},
    {0x03000019u},
    {0x00000012u},
    {0x80000002u},
    {0x80000001u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_cassius_3[13] = {
    {0x00004237u},
    {0x03000019u},
    {0x00000013u},
    {0x80000004u},
    {0x80000001u},
    {0x3FFFFFFEu},
    {0x01000032u},
    {(unsigned int)do_my_fatality},
    {0x80000003u},
    {0x80000004u},
    {0x80000002u},
    {0x80000001u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_cassius_4[6] = {
    {0x00004235u},
    {0x03000019u},
    {0x0000000Eu},
    {0x80000002u},
    {0x80000001u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_shujinko_1[18] = {
    {0x00001203u},
    {0x0300001Bu},
    {0x00000019u},
    {0x80000002u},
    {0x80000001u},
    {0x00001205u},
    {0x03000019u},
    {0x0000001Cu},
    {0x80000004u},
    {0x80000001u},
    {0x3FFFFFFEu},
    {0x01000032u},
    {(unsigned int)do_my_2nd_fatality},
    {0x80000002u},
    {0x80000003u},
    {0x80000001u},
    {0x80000001u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_shujinko_2[11] = {
    {0x00004241u},
    {0x03000019u},
    {0x00000018u},
    {0x80000004u},
    {0x80000001u},
    {0x0000421Eu},
    {0x03000019u},
    {0x00000016u},
    {0x80000001u},
    {0x80000001u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_shujinko_3[18] = {
    {0x00004242u},
    {0x03000019u},
    {0x0000001Eu},
    {0x80000004u},
    {0x80000002u},
    {0x0000421Bu},
    {0x03000019u},
    {0x0000000Du},
    {0x80000001u},
    {0x80000002u},
    {0x3FFFFFFEu},
    {0x01000032u},
    {(unsigned int)do_my_fatality},
    {0x80000003u},
    {0x80000004u},
    {0x80000004u},
    {0x80000001u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_shujinko_4[12] = {
    {0x00004219u},
    {0x03000028u},
    {0x00000010u},
    {0x80000002u},
    {0x80000004u},
    {0x80000002u},
    {0x00004239u},
    {0x03000019u},
    {0x00000023u},
    {0x80000001u},
    {0x80000001u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_dk_1[16] = {
    {0x0000423Eu},
    {0x03000019u},
    {0x0000001Du},
    {0x80000001u},
    {0x80000001u},
    {0x0000423Cu},
    {0x03000019u},
    {0x00000013u},
    {0x80000001u},
    {0x80000004u},
    {0x0000423Bu},
    {0x03000019u},
    {0x0000000Eu},
    {0x80000001u},
    {0x80000002u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_dk_2[6] = {
    {0x0000423Du},
    {0x03000019u},
    {0x0000001Cu},
    {0x80000001u},
    {0x80000001u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_dk_3[6] = {
    {0x0000423Fu},
    {0x03000019u},
    {0x0000001Fu},
    {0x80000001u},
    {0x80000001u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_shaokahn_1[11] = {
    {0x00004255u},
    {0x03000019u},
    {0x0000000Cu},
    {0x80000004u},
    {0x80000001u},
    {0x00004258u},
    {0x03000019u},
    {0x00000010u},
    {0x80000004u},
    {0x80000002u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_shaokahn_1a[6] = {
    {0x00000000u},
    {0x03000019u},
    {0x00000018u},
    {0x80000004u},
    {0x80000003u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_shaokahn_2[20] = {
    {0x00004256u},
    {0x03000019u},
    {0x00000011u},
    {0x80000001u},
    {0x80000001u},
    {0x3FFFFFFEu},
    {0x01000032u},
    {(unsigned int)do_my_2nd_fatality},
    {0x80000002u},
    {0x80000001u},
    {0x80000004u},
    {0x80000001u},
    {0x3FFFFFFEu},
    {0x01000032u},
    {(unsigned int)do_my_fatality},
    {0x80000004u},
    {0x80000003u},
    {0x80000003u},
    {0x80000001u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_shaokahn_3[6] = {
    {0x00004257u},
    {0x0300001Eu},
    {0x00000013u},
    {0x80000002u},
    {0x80000001u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_goro_1[13] = {
    {0x00004254u},
    {0x03000019u},
    {0x00000012u},
    {0x80000004u},
    {0x80000002u},
    {0x3FFFFFFEu},
    {0x01000032u},
    {(unsigned int)do_my_2nd_fatality},
    {0x80000001u},
    {0x80000002u},
    {0x80000001u},
    {0x80000002u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_goro_2[6] = {
    {0x00000000u},
    {0x03000019u},
    {0x0000000Du},
    {0x80000004u},
    {0x80000001u},
    {0xFFFFFFFFu},
};

MovesScanWord scan_goro_3[18] = {
    {0x00004259u},
    {0x03000023u},
    {0x00000008u},
    {0x80000003u},
    {0x80000004u},
    {0x00004259u},
    {0x03000019u},
    {0x00000009u},
    {0x80000004u},
    {0x80000003u},
    {0x3FFFFFFEu},
    {0x01000032u},
    {(unsigned int)do_my_fatality},
    {0x80000002u},
    {0x80000002u},
    {0x80000001u},
    {0x80000001u},
    {0xFFFFFFFFu},
};
