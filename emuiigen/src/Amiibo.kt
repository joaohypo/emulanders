package com.xortroll.emuiibo.emuiigen

import java.nio.file.Paths
import java.io.File
import java.io.FileInputStream
import java.io.FileWriter
import kotlin.random.Random
import org.json.JSONObject
import org.json.JSONArray
import org.apache.commons.io.IOUtils

data class Amiibo(var first_write_date: AmiiboDate, var id: AmiiboId, var last_write_date: AmiiboDate, var mii_charinfo_file: String, var name: String, var uuid: UByteArray, var use_random_uuid: Boolean, var version: UInt, var write_counter: UShort, var areas: AmiiboAreaInfo?) {
    companion object {
        val UuidLength = 10;
        val NameMaxLength = 10;

        fun randomUuid() : UByteArray {
            val uuid = ByteArray(UuidLength);
            Random.nextBytes(uuid, 0, 7);
            return uuid.toUByteArray();
        }

        fun tryParse(path: String) : Pair<AmiiboStatus, Amiibo?> {
            var status: AmiiboStatus = AmiiboStatus.of(AmiiboStatusKind.Ok);
            var areas: AmiiboAreaInfo?;
    
            val flag_path = Paths.get(path, "amiibo.flag").toAbsolutePath().toString();
            val json_path = Paths.get(path, "amiibo.json").toAbsolutePath().toString();
            val areas_json_path = Paths.get(path, "areas.json").toAbsolutePath().toString();
    
            val flag_file = File(flag_path);
            if(!flag_file.isFile) {
                status = AmiiboStatus.of(AmiiboStatusKind.FlagNotFound);
            }
    
            try {
                val areas_json_strm = FileInputStream(areas_json_path);
                val areas_json_raw = IOUtils.toString(areas_json_strm);
                val areas_json = JSONObject(areas_json_raw);
    
                areas = AmiiboAreaInfo.fromJson(areas_json);
            }
            catch(ex: Exception) {
                areas = null;
            }

            try {
                val json_strm = FileInputStream(json_path);
                val json_raw = IOUtils.toString(json_strm);
                val json = JSONObject(json_raw);
    
                val first_write_date = AmiiboDate.fromJson(json.getJSONObject("first_write_date"));
                val id = AmiiboId.fromJson(json.getJSONObject("id"));
                val last_write_date = AmiiboDate.fromJson(json.getJSONObject("last_write_date"));
    
                val mii_charinfo_file = json.getString("mii_charinfo_file");
    
                val mii_charinfo_path = Paths.get(path, mii_charinfo_file).toAbsolutePath().toString();
                val mii_charinfo_file_v = File(mii_charinfo_path);
                if(!mii_charinfo_file_v.isFile) {
                    status = status and AmiiboStatusKind.MiiCharInfoNotFound;
                }
    
                val name = json.getString("name");
                if(name.length > NameMaxLength) {
                    status = status and AmiiboStatusKind.InvalidNameLength;
                }
    
                val uuid_array = json.getJSONArray("uuid");
                if(uuid_array.length() != UuidLength) {
                    return Pair(AmiiboStatus.of(AmiiboStatusKind.InvalidUuidLength), null);
                }
    
                val uuid = mutableListOf<UByte>();
                for(i in 0 until uuid_array.length()) {
                    uuid.add(uuid_array.getInt(i).toUByte());
                }
                
                val use_random_uuid = json.getBoolean("use_random_uuid");
                val version = json.getInt("version").toUInt();
                val write_counter = json.getInt("write_counter").toUShort();
    
                val amiibo = Amiibo(first_write_date, id, last_write_date, mii_charinfo_file, name, uuid.toUByteArray(), use_random_uuid, version, write_counter, areas)
                return Pair(status, amiibo);
            }
            catch(ex: Exception) {
                System.out.println("Exception loading amiibo JSON: " + ex.toString());
                return Pair(AmiiboStatus.of(AmiiboStatusKind.JsonNotFound), null);
            }
        }
    }

    inline fun hasAreas() : Boolean {
        return this.areas != null;
    }

    fun save(path: String) : Boolean {
        try {
            val path_v = File(path);
            path_v.mkdirs();

            val flag_path = Paths.get(path, "amiibo.flag").toAbsolutePath().toFile();
            flag_path.createNewFile();

            if(this.hasAreas()) {
                val areas_json = this.areas!!.toJson();
                val areas_json_path = Paths.get(path, "areas.json").toAbsolutePath().toString();
                val areas_json_w = FileWriter(areas_json_path);
                areas_json_w.write(areas_json.toString(4));
                areas_json_w.flush();
                areas_json_w.close();
            }

            val json = JSONObject();
            json.put("first_write_date", this.first_write_date.toJson());
            json.put("id", this.id.toJson());
            json.put("last_write_date", this.last_write_date.toJson());
            json.put("mii_charinfo_file", this.mii_charinfo_file);
            json.put("name", this.name);

            val uuid_array = JSONArray();
            for(i in 0 until this.uuid.size) {
                uuid_array.put(this.uuid[i].toInt());
            }
            json.put("uuid", uuid_array);

            json.put("use_random_uuid", this.use_random_uuid);
            json.put("version", this.version.toInt());
            json.put("write_counter", this.write_counter.toInt());

            val json_path = Paths.get(path, "amiibo.json").toAbsolutePath().toString();
            val json_w = FileWriter(json_path);
            json_w.write(json.toString(4));
            json_w.flush();
            json_w.close();
        }
        catch(ex: Exception) {
            System.out.println("Exception saving amiibo: " + ex.toString());
            return false;
        }
        
        return true;
    }
}