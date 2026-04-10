package com.xortroll.emuiibo.emuiigen

import org.json.JSONObject

data class AmiiboAreaEntry(val program_id: ULong, val access_id: UInt) {
    companion object {
        fun fromJson(json: JSONObject) : AmiiboAreaEntry {
            val program_id = json.getLong("program_id").toULong();
            val access_id = json.getInt("access_id").toUInt();
            return AmiiboAreaEntry(program_id, access_id);
        }
    }

    fun toJson() : JSONObject {
        val json_obj = JSONObject();
        json_obj.put("program_id", this.program_id);
        json_obj.put("access_id", this.access_id);
        return json_obj;
    }
}