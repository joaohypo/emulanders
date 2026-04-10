package com.xortroll.emuiibo.emuiigen

import org.apache.commons.io.FileUtils
import org.apache.commons.io.FilenameUtils
import org.apache.commons.io.IOUtils
import org.apache.commons.net.ftp.FTPClient
import org.apache.commons.net.ftp.FTPReply
import java.net.URL
import java.io.File
import java.io.FileOutputStream
import java.nio.charset.StandardCharsets
import java.nio.channels.Channels
import java.text.Normalizer

class Utils {
    companion object {
        fun reverseUShort(ush: UShort) : UShort {
            val byte1 = ush.toByte();
            val byte2 = (ush.toInt() ushr 8).toByte();

            return (
                ((byte2.toInt() and 0xFF)) or
                ((byte1.toInt() and 0xFF) shl  8)
            ).toUShort()
        }

        fun netDownloadString(url: String) : String {
            val connection = URL(url).openConnection().apply {
                connectTimeout = 10000 // connectTimeoutMs
                readTimeout = 10000 // readTimeoutMs
            }

            connection.getInputStream().use { input ->
                return IOUtils.toString(input, Charsets.UTF_8)
            }
        }

        fun netDownloadFile(url: String, path: String) {
            FileUtils.copyURLToFile(URL(url), File(path), 10000, 10000);
        }

        fun unaccentString(str: String) : String {
            return Normalizer.normalize(str, Normalizer.Form.NFD).replace("[^\\p{ASCII}]".toRegex(), "");
        }

        fun ensureValidFileDirectoryName(str: String) : String {
            // Ensure no trailing dots (for NTFS)
            val str_no_trailing_dots = str.trimEnd('.');

            // Ensure valid ASCII (for Switch filesystem)
            val str_no_accents = unaccentString(str_no_trailing_dots);

            return str_no_accents;
        }

        fun truncateString(str: String, len: Int) : String {
            if(str.length > len) {
                return str.substring(0, len);
            }
            else {
                return str;
            }
        }

        fun ensureFtpDirectory(client: FTPClient, dir: String) : Boolean {
            var dir_exists = true;

            val items = dir.split("/");
            for(item in items) {
                if(!item.isEmpty()) {
                    if(dir_exists) {
                        dir_exists = client.changeWorkingDirectory(item);
                    }
                    if(!dir_exists) {
                        if(!client.makeDirectory(item)) {
                            return false;
                        }
                        if(!client.changeWorkingDirectory(item)) {
                            return false;
                        }
                    }
                }
            }

            return true;
        }

        val RemovableAmiiboNameCharacters = charArrayOf(' ', '-', '.', '\'');

        fun produceAmiiboName(base_name: String) : String {
            // To retain as much of the name as possible when clamping to the limit, remove spaces and other less important ones
            var name = base_name;
            for(ch in RemovableAmiiboNameCharacters) {
                name = name.replace(ch.toString(), "");
            }

            // Now apply the limit
            name = truncateString(name, Amiibo.NameMaxLength);

            return name;
        }
    }
}