package com.xortroll.emuiibo.emuiigen

import org.json.JSONObject

data class AmiiboId(val game_character_id: UShort, val character_variant: UByte, val figure_type: UByte, val series: UByte, val model_number: UShort) {
    companion object {
        fun fromJson(json: JSONObject) : AmiiboId {
            val game_character_id = json.getInt("game_character_id").toUShort();
            val character_variant = json.getInt("character_variant").toUByte();
            val figure_type = json.getInt("figure_type").toUByte();
            val series = json.getInt("series").toUByte();
            val model_number = json.getInt("model_number").toUShort();
            
            return AmiiboId(game_character_id, character_variant, figure_type, series, model_number);
        }
    }
    
    fun toJson() : JSONObject {
        val json_obj = JSONObject();
        json_obj.put("game_character_id", this.game_character_id.toInt());
        json_obj.put("character_variant", this.character_variant.toInt());
        json_obj.put("figure_type", this.figure_type.toInt());
        json_obj.put("series", this.series.toInt());
        json_obj.put("model_number", this.model_number.toInt());
        return json_obj;
    }
}