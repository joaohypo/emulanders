package com.xortroll.emuiibo.emuiigen

import org.json.JSONObject
import org.json.JSONArray

data class AmiiboAreaInfo(val current_area_access_id: UInt, val areas: List<AmiiboAreaEntry>) {
    companion object {
        fun fromJson(json: JSONObject) : AmiiboAreaInfo {
            val current_area_access_id = json.getInt("current_area_access_id").toUInt();
            
            val areas_array = json.getJSONArray("areas");
            val areas = mutableListOf<AmiiboAreaEntry>();
            for(area in areas_array) {
                val area_obj = area as JSONObject;
                areas.add(AmiiboAreaEntry.fromJson(area_obj));
            }

            return AmiiboAreaInfo(current_area_access_id, areas);
        }
    }

    fun toJson() : JSONObject {
        val json_obj = JSONObject();
        json_obj.put("current_area_access_id", this.current_area_access_id);
        val areas_array = JSONArray();
        for(area in areas) {
            areas_array.put(area.toJson());
        }
        json_obj.put("areas", areas_array);
        return json_obj;
    }
}