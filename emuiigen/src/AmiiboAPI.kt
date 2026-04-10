package com.xortroll.emuiibo.emuiigen

import org.json.JSONObject
import org.json.JSONArray
import java.io.File
import java.io.FileWriter
import java.io.FileInputStream
import java.nio.file.Path
import java.nio.file.Paths
import org.apache.commons.io.IOUtils

class AmiiboAPI {
    companion object {
        val DefaultUrl = "https://www.amiiboapi.org/api/amiibo/";

        fun parseApiJson(json_data: JSONObject) : Map<String, List<AmiiboAPIEntry>>? {
            val json_entries = json_data.getJSONArray("amiibo");

            val entry_map = mutableMapOf<String, MutableList<AmiiboAPIEntry>>();
            for(i in 0 until json_entries.length()) {
                val entry = AmiiboAPIEntry.fromJson(json_entries.getJSONObject(i));
                val series = entry_map.get(entry.series_name)?.toMutableList();
                series?.let {
                    series.add(entry);
                    entry_map.put(entry.series_name, series);
                }
                ?: let {
                    entry_map.put(entry.series_name, mutableListOf(entry));
                }
            }
            for(bass in entry_map.values) {
                bass.sortWith(compareBy(AmiiboAPIEntry::amiibo_name));
            }
            return entry_map;
        }

        fun readApi(url: String) : Map<String, List<AmiiboAPIEntry>>? {
            val cur_path = File(this::class.java.getProtectionDomain().getCodeSource().getLocation().toURI()).parentFile.absolutePath;
            val local_api_json_path = Paths.get(cur_path, "api.json").toString();
            System.out.println("AmiiboAPI: Downloading API from " + url);

            try {
                val raw_json = Utils.netDownloadString(url);
                val json_data = JSONObject(raw_json);

                try {
                    System.out.println("AmiiboAPI: Saving local copy of API to " + local_api_json_path);
                    val json_data_w = FileWriter(local_api_json_path);
                    json_data_w.write(json_data.toString(4));
                    json_data_w.flush();
                    json_data_w.close();
                }
                catch(ex: Exception) {
                    System.out.println("Exception saving AmiiboAPI as local JSON: " + ex.toString());
                }

                return parseApiJson(json_data);
            }
            catch(ex: Exception) {
                System.out.println("Exception reading AmiiboAPI: " + ex.toString());

                try {
                    // Try loading local saved API
                    val json_data_strm = FileInputStream(local_api_json_path);
                    val json_data_raw = IOUtils.toString(json_data_strm);
                    val json_data = JSONObject(json_data_raw);
                    return parseApiJson(json_data);
                }
                catch(ex: Exception) {
                    System.out.println("Exception reading local API JSON: " + ex.toString());
                    return null;
                }
            }
        }
    }
}