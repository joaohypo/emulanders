package com.xortroll.emuiibo.emuiigen

import org.json.JSONObject

data class AmiiboAPIEntry(val id: AmiiboId, val amiibo_name: String, val series_name: String, val character_name: String, val image_url: String) {
    companion object {
        fun fromJson(json: JSONObject) : AmiiboAPIEntry {
            val amiibo_name = json.getString("name");
            val series_name = json.getString("amiiboSeries");
            val character_name = json.getString("character");
            val image_url = json.getString("image");

            val id_head = json.getString("head");
            val id_tail = json.getString("tail");
            val id_str = id_head + id_tail;
            var parse_offset = 0;

            val game_character_id_be_str = id_str.substring(parse_offset, parse_offset + 4);
            parse_offset += 4;
            val game_character_id = Utils.reverseUShort(game_character_id_be_str.toUShort(16));

            val character_variant_str = id_str.substring(parse_offset, parse_offset + 2);
            parse_offset += 2;
            val character_variant = character_variant_str.toUByte(16);
        
            val figure_type_str = id_str.substring(parse_offset, parse_offset + 2);
            parse_offset += 2;
            val figure_type = figure_type_str.toUByte(16);

            val model_number_str = id_str.substring(parse_offset, parse_offset + 4);
            parse_offset += 4;
            val model_number = model_number_str.toUShort(16);

            val series_str = id_str.substring(parse_offset, parse_offset + 2);
            parse_offset += 2;
            val series = series_str.toUByte(16);

            val id = AmiiboId(game_character_id, character_variant, figure_type, series, model_number);
            return AmiiboAPIEntry(id, amiibo_name, series_name, character_name, image_url);
        }
    }
}