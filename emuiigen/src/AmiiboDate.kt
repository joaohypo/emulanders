package com.xortroll.emuiibo.emuiigen

import org.json.JSONObject

data class AmiiboDate(val year: UShort, val month: UByte, val day: UByte) {
    companion object {
        fun fromJson(json: JSONObject) : AmiiboDate {
            val year = json.getInt("y").toUShort();
            val month = json.getInt("m").toUByte();
            val day = json.getInt("d").toUByte();
            return AmiiboDate(year, month, day);
        }
    }

    fun toJson() : JSONObject {
        val json_obj = JSONObject();
        json_obj.put("y", this.year.toInt());
        json_obj.put("m", this.month.toInt());
        json_obj.put("d", this.day.toInt());
        return json_obj;
    }
}