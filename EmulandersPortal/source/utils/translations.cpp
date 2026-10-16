#include "translations.hpp"
#include <cstring>
#include <string>

static const LanguageOption kSupportedLanguages[] = {
    { AppLanguage::EN,      "en",      "English" },
    { AppLanguage::PT_BR,   "pt-BR",   "Português (BR)" },
    { AppLanguage::ES,      "es",      "Español" },
    { AppLanguage::FR,      "fr",      "Français" },
    { AppLanguage::DE,      "de",      "Deutsch" },
    { AppLanguage::IT,      "it",      "Italiano" },
    { AppLanguage::RU,      "ru",      "Русский" },
    { AppLanguage::KO,      "ko",      "한국어" },
    { AppLanguage::ZH_HANS, "zh-Hans", "简体中文" },
    { AppLanguage::ZH_HANT, "zh-Hant", "繁體中文" }
};

const LanguageOption* GetSupportedLanguages(int* out_count) {
    if (out_count) *out_count = (int)(sizeof(kSupportedLanguages) / sizeof(kSupportedLanguages[0]));
    return kSupportedLanguages;
}

const char* LanguageToCode(AppLanguage lang) {
    for (const auto& opt : kSupportedLanguages) {
        if (opt.lang == lang) return opt.code;
    }
    return "en";
}

const char* LanguageToDisplayName(AppLanguage lang) {
    for (const auto& opt : kSupportedLanguages) {
        if (opt.lang == lang) return opt.display_name;
    }
    return "English";
}

AppLanguage CodeToLanguage(const char* code) {
    if (!code || !code[0]) return AppLanguage::EN;
    std::string s(code);
    if (s.find("pt") != std::string::npos) return AppLanguage::PT_BR;
    if (s.find("es") != std::string::npos) return AppLanguage::ES;
    if (s.find("fr") != std::string::npos) return AppLanguage::FR;
    if (s.find("de") != std::string::npos) return AppLanguage::DE;
    if (s.find("it") != std::string::npos) return AppLanguage::IT;
    if (s.find("ru") != std::string::npos) return AppLanguage::RU;
    if (s.find("ko") != std::string::npos) return AppLanguage::KO;
    if (s.find("zh-Hant") != std::string::npos || s.find("zh-TW") != std::string::npos || s.find("zh-HK") != std::string::npos)
        return AppLanguage::ZH_HANT;
    if (s.find("zh") != std::string::npos) return AppLanguage::ZH_HANS;
    return AppLanguage::EN;
}

struct TranslationEntry {
    const char* en;
    const char* es;
    const char* fr;
    const char* de;
    const char* it;
    const char* ru;
    const char* ko;
    const char* zh_hans;
    const char* zh_hant;
};

static const TranslationEntry kTranslations[] = {
    // --- Main Menu & Headers ---
    {
        "Skylander Dumper",
        "Lector de Skylanders",
        "Lecteur de Skylanders",
        "Skylander-Leser",
        "Lettore Skylander",
        "Сканер Skylanders",
        "스카이랜더 덤퍼",
        "Skylander 读取器",
        "Skylander 讀取器"
    },
    {
        "File Manager & Dump Cloning",
        "Gestor de Archivos y Clonación",
        "Gestionnaire de Fichiers",
        "Dateimanager & Klonen",
        "Gestione File e Clonazione",
        "Файловый менеджер",
        "파일 관리 및 복제",
        "文件管理与克隆",
        "檔案管理與複製"
    },
    {
        "Figure Image Editor",
        "Editor de Imágenes",
        "Éditeur d'Images",
        "Figuren-Bildeditor",
        "Editor Immagini",
        "Редактор обложек",
        "피규어 이미지 편집기",
        "角色封面编辑器",
        "角色封面編輯器"
    },
    {
        "Dump Downloader",
        "Descarga de Dumps",
        "Téléchargement de Dumps",
        "Dump-Downloader",
        "Download Dump",
        "Загрузка дампов",
        "덤프 다운로더",
        "角色数据下载",
        "角色資料下載"
    },
    {
        "Asset Downloader",
        "Descarga de Assets",
        "Téléchargement d'Assets",
        "Asset-Downloader",
        "Download Immagini",
        "Загрузка обложек",
        "에셋 다운로더",
        "图包资源下载",
        "圖包資源下載"
    },
    {
        "Settings",
        "Ajustes",
        "Paramètres",
        "Einstellungen",
        "Impostazioni",
        "Настройки",
        "설정",
        "设置",
        "設定"
    },
    {
        "Download Complete",
        "Descarga Completada",
        "Téléchargement Terminé",
        "Download Abgeschlossen",
        "Download Completato",
        "Загрузка завершена",
        "다운로드 완료",
        "下载完成",
        "下載完成"
    },
    {
        "DISMISS",
        "CERRAR",
        "FERMER",
        "SCHLIESSEN",
        "CHIUDI",
        "ЗАКРЫТЬ",
        "닫기",
        "关闭",
        "關閉"
    },
    {
        "[D-Pad] Navigate  |  [A] Confirm  |  [B] Back  |  [+] Exit",
        "[D-Pad] Navegar  |  [A] Confirmar  |  [B] Volver  |  [+] Salir",
        "[D-Pad] Naviguer  |  [A] Confirmer  |  [B] Retour  |  [+] Quitter",
        "[D-Pad] Navigieren  |  [A] Bestätigen  |  [B] Zurück  |  [+] Beenden",
        "[D-Pad] Naviga  |  [A] Conferma  |  [B] Indietro  |  [+] Esci",
        "[D-Pad] Навигация  |  [A] Выбор  |  [B] Назад  |  [+] Выход",
        "[D-Pad] 이동  |  [A] 확인  |  [B] 뒤로  |  [+] 종료",
        "[方向键] 导航  |  [A] 确认  |  [B] 返回  |  [+] 退出",
        "[方向鍵] 導覽  |  [A] 確認  |  [B] 返回  |  [+] 退出"
    },
    {
        "[D-Pad / L-Stick] Move  |  [R-Stick / L / R] Zoom  |  [A] Save  |  [X] Reset  |  [B] Back",
        "[D-Pad / L-Stick] Mover  |  [R-Stick / L / R] Zoom  |  [A] Guardar  |  [X] Reiniciar  |  [B] Volver",
        "[D-Pad / L-Stick] Déplacer  |  [R-Stick / L / R] Zoom  |  [A] Sauver  |  [X] Réinit.  |  [B] Retour",
        "[D-Pad / L-Stick] Bewegen  |  [R-Stick / L / R] Zoom  |  [A] Speichern  |  [X] Reset  |  [B] Zurück",
        "[D-Pad / L-Stick] Sposta  |  [R-Stick / L / R] Zoom  |  [A] Salva  |  [X] Reset  |  [B] Indietro",
        "[D-Pad / L] Двигать  |  [R / L / R] Масштаб  |  [A] Сохр.  |  [X] Сброс  |  [B] Назад",
        "[D-Pad / L스틱] 이동  |  [R스틱 / L / R] 줌  |  [A] 저장  |  [X] 초기화  |  [B] 뒤로",
        "[方向键 / 左摇杆] 移动  |  [右摇杆 / L / R] 缩放  |  [A] 保存  |  [X] 重置  |  [B] 返回",
        "[方向鍵 / 左搖桿] 移動  |  [右搖桿 / L / R] 縮放  |  [A] 儲存  |  [X] 重置  |  [B] 返回"
    },
    {
        "[D-Pad] Navigate  |  [R-Stick / Touch] Scroll  |  [A] Confirm  |  [B] Back  |  [+] Exit",
        "[D-Pad] Navegar  |  [R-Stick / Táctil] Desplazar  |  [A] Confirmar  |  [B] Volver  |  [+] Salir",
        "[D-Pad] Naviguer  |  [R-Stick / Tactile] Défiler  |  [A] Confirmer  |  [B] Retour  |  [+] Quitter",
        "[D-Pad] Navigieren  |  [R-Stick / Touch] Scrollen  |  [A] Bestätigen  |  [B] Zurück  |  [+] Beenden",
        "[D-Pad] Naviga  |  [R-Stick / Touch] Scorri  |  [A] Conferma  |  [B] Indietro  |  [+] Esci",
        "[D-Pad] Навигация  |  [R-стик / Тач] Прокрутка  |  [A] Выбор  |  [B] Назад  |  [+] Выход",
        "[D-Pad] 이동  |  [R스틱 / 터치] 스크롤  |  [A] 확인  |  [B] 뒤로  |  [+] 종료",
        "[方向键] 导航  |  [右摇杆 / 触屏] 滚动  |  [A] 确认  |  [B] 返回  |  [+] 退出",
        "[方向鍵] 導覽  |  [右搖桿 / 觸控] 捲動  |  [A] 確認  |  [B] 返回  |  [+] 退出"
    },

    // --- Screen 0: Skylander Dumper ---
    {
        "Place a physical Skylander on the Right Joy-Con NFC reader.",
        "Coloca un Skylander físico sobre el lector NFC del Joy-Con derecho.",
        "Placez un Skylander physique sur le lecteur NFC du Joy-Con droit.",
        "Stelle eine Skylander-Figur auf den NFC-Leser des rechten Joy-Con.",
        "Posiziona uno Skylander fisico sul lettore NFC del Joy-Con destro.",
        "Поместите фигурку Skylander на NFC-считыватель правого Joy-Con.",
        "오른쪽 Joy-Con NFC 리더기 위에 스카이랜더 피규어를 올려주세요.",
        "请将实体 Skylander 角色放置在右侧 Joy-Con 的 NFC 感应区上。",
        "請將實體 Skylander 角色放置在右側 Joy-Con 的 NFC 感應區上。"
    },
    {
        "Keep it still and centered on the analog stick.",
        "Mantenlo quieto y centrado sobre el stick analógico.",
        "Gardez-le immobile et centré sur le stick analogique.",
        "Halte sie ruhig und mittig auf dem Analogstick.",
        "Tienilo fermo e centrato sullo stick analogico.",
        "Держите фигурку неподвижно по центру стика.",
        "아날로그 스틱 중앙에 흔들리지 않게 유지해 주세요.",
        "请保持角色平稳并居中贴合在摇杆上方。",
        "請保持角色平穩並置中貼合在搖桿上方。"
    },
    {
        "Note: Some third-party controllers only support Amiibo, not Skylander.",
        "Nota: Algunos mandos genéricos solo soportan Amiibo, no Skylander.",
        "Note : Certaines manettes tierces ne supportent que les Amiibo.",
        "Hinweis: Manche Drittanbieter-Controller unterstützen nur Amiibo.",
        "Nota: Alcuni controller non originali supportano solo Amiibo.",
        "Примечание: Некоторые сторонние геймпады поддерживают только Amiibo.",
        "참고: 일부 서드파티 컨트롤러는 아미보만 지원합니다.",
        "注意：部分第三方手柄仅支持 Amiibo，不支持 Skylander 协议。",
        "注意：部分副廠手把僅支援 Amiibo，不支援 Skylander 協定。"
    },
    {
        "Start Scanning",
        "Iniciar Lectura",
        "Démarrer la Lecture",
        "Scan Starten",
        "Avvia Scansione",
        "Начать сканирование",
        "스캔 시작",
        "开始读取",
        "開始讀取"
    },
    {
        "Initializing NFC hardware...",
        "Inicializando hardware NFC...",
        "Initialisation du matériel NFC...",
        "NFC-Hardware wird initialisiert...",
        "Inizializzazione hardware NFC...",
        "Инициализация NFC...",
        "NFC 하드웨어 초기화 중...",
        "正在初始化 NFC 硬件...",
        "正在初始化 NFC 硬體..."
    },
    {
        "Waiting for Skylander...",
        "Esperando Skylander...",
        "En attente du Skylander...",
        "Warte auf Skylander...",
        "In attesa dello Skylander...",
        "Ожидание фигурки Skylander...",
        "스카이랜더 대기 중...",
        "正在等待 Skylander 角色...",
        "正在等待 Skylander 角色..."
    },
    {
        "Place the figure on the NFC reader now.",
        "Coloca la figura en el lector NFC ahora.",
        "Placez la figurine sur le lecteur NFC maintenant.",
        "Stelle die Figur jetzt auf den NFC-Leser.",
        "Posiziona ora la statuetta sul lettore NFC.",
        "Приложите фигурку к NFC-считывателю.",
        "지금 피규어를 NFC 리더기에 올려주세요.",
        "请现在将角色放到 NFC 感应区上。",
        "請現在將角色放到 NFC 感應區上。"
    },
    {
        "Reading data... %d/64 blocks",
        "Leyendo datos... %d/64 bloques",
        "Lecture des données... %d/64 blocs",
        "Lese Daten... %d/64 Blöcke",
        "Lettura dati... %d/64 blocchi",
        "Чтение данных... %d/64 блоков",
        "데이터 읽는 중... %d/64 블록",
        "正在读取数据... %d/64 块",
        "正在讀取資料... %d/64 區塊"
    },
    {
        "Skylander detected!",
        "¡Skylander detectado!",
        "Skylander détecté !",
        "Skylander erkannt!",
        "Skylander rilevato!",
        "Фигурка обнаружена!",
        "스카이랜더 감지됨!",
        "已识别 Skylander！",
        "已識別 Skylander！"
    },
    {
        "Identified",
        "Identificado",
        "Identifié",
        "Identifiziert",
        "Identificato",
        "Определен",
        "인식됨",
        "已识别",
        "已識別"
    },
    {
        "Save with Official Name",
        "Guardar con Nombre Oficial",
        "Sauver avec le Nom Officiel",
        "Mit offiziellem Namen speichern",
        "Salva con Nome Ufficiale",
        "Сохранить с офиц. именем",
        "공식 이름으로 저장",
        "使用官方名称保存",
        "使用官方名稱儲存"
    },
    {
        "Save with Custom Name",
        "Guardar con Nombre Personalizado",
        "Sauver avec un Nom Personnalisé",
        "Mit eigenem Namen speichern",
        "Salva con Nome Personalizzato",
        "Сохранить со своим именем",
        "사용자 지정 이름으로 저장",
        "自定义名称保存",
        "自訂名稱儲存"
    },
    {
        "Enter filename",
        "Introduce el nombre del archivo",
        "Entrez le nom du fichier",
        "Dateinamen eingeben",
        "Inserisci nome file",
        "Введите имя файла",
        "파일 이름 입력",
        "输入文件名",
        "輸入檔案名稱"
    },
    {
        "Cancel",
        "Cancelar",
        "Annuler",
        "Abbrechen",
        "Annulla",
        "Отмена",
        "취소",
        "取消",
        "取消"
    },
    {
        "Dump saved successfully!",
        "¡Dump guardado con éxito!",
        "Dump sauvegardé avec succès !",
        "Dump erfolgreich gespeichert!",
        "Dump salvato con successo!",
        "Дамп успешно сохранен!",
        "덤프가 성공적으로 저장되었습니다!",
        "角色数据保存成功！",
        "角色資料儲存成功！"
    },
    {
        "Scan Another",
        "Leer Otro",
        "Scanner un Autre",
        "Weiteren Scannen",
        "Scansiona un Altro",
        "Сканировать еще",
        "다른 피규어 스캔",
        "继续读取下一个",
        "繼續讀取下一個"
    },
    {
        "Scan Error",
        "Error de Lectura",
        "Erreur de Lecture",
        "Scan-Fehler",
        "Errore di Scansione",
        "Ошибка чтения",
        "스캔 오류",
        "读取错误",
        "讀取錯誤"
    },
    {
        "Try Again",
        "Intentar de Nuevo",
        "Réessayer",
        "Erneut versuchen",
        "Riprova",
        "Повторить",
        "다시 시도",
        "重试",
        "重試"
    },
    {
        "File name cannot be empty!",
        "¡El nombre no puede estar vacío!",
        "Le nom du fichier ne peut pas être vide !",
        "Dateiname darf nicht leer sein!",
        "Il nome del file non può essere vuoto!",
        "Имя файла не может быть пустым!",
        "파일 이름은 비워둘 수 없습니다!",
        "文件名不能为空！",
        "檔案名稱不能為空！"
    },

    // --- Screen 1: File Manager ---
    {
        "Source: ★ Tesla Overlay Favorites (sdmc:/emulanders/overlay/favorites.txt)",
        "Origen: ★ Favoritos del Overlay (sdmc:/emulanders/overlay/favorites.txt)",
        "Source : ★ Favoris de l'Overlay (sdmc:/emulanders/overlay/favorites.txt)",
        "Quelle: ★ Overlay-Favoriten (sdmc:/emulanders/overlay/favorites.txt)",
        "Origine: ★ Preferiti Overlay (sdmc:/emulanders/overlay/favorites.txt)",
        "Источник: ★ Избранное оверлея (sdmc:/emulanders/overlay/favorites.txt)",
        "원본: ★ 오버레이 즐겨찾기 (sdmc:/emulanders/overlay/favorites.txt)",
        "来源：★ Tesla 悬浮窗收藏夹 (sdmc:/emulanders/overlay/favorites.txt)",
        "來源：★ Tesla 懸浮窗最愛 (sdmc:/emulanders/overlay/favorites.txt)"
    },
    {
        "Path:",
        "Ruta:",
        "Chemin :",
        "Pfad:",
        "Percorso:",
        "Путь:",
        "경로:",
        "路径：",
        "路徑："
    },
    {
        "(L) Refresh",
        "(L) Actualizar",
        "(L) Actualiser",
        "(L) Aktualisieren",
        "(L) Aggiorna",
        "(L) Обновить",
        "(L) 새로고침",
        "(L) 刷新",
        "(L) 重新整理"
    },
    {
        "(R) New Folder",
        "(R) Nueva Carpeta",
        "(R) Nouveau Dossier",
        "(R) Neuer Ordner",
        "(R) Nuova Cartella",
        "(R) Новая папка",
        "(R) 새 폴더",
        "(R) 新建文件夹",
        "(R) 新增資料夾"
    },
    {
        "Folder name",
        "Nombre de la carpeta",
        "Nom du dossier",
        "Ordnername",
        "Nome cartella",
        "Имя папки",
        "폴더 이름",
        "文件夹名称",
        "資料夾名稱"
    },
    {
        "[Y] ★ Favorites",
        "[Y] ★ Favoritos",
        "[Y] ★ Favoris",
        "[Y] ★ Favoriten",
        "[Y] ★ Preferiti",
        "[Y] ★ Избранное",
        "[Y] ★ 즐겨찾기",
        "[Y] ★ 收藏夹",
        "[Y] ★ 最愛"
    },
    {
        "Name",
        "Nombre",
        "Nom",
        "Name",
        "Nome",
        "Имя",
        "이름",
        "名称",
        "名稱"
    },
    {
        "Size",
        "Tamaño",
        "Taille",
        "Größe",
        "Dimensione",
        "Размер",
        "크기",
        "大小",
        "大小"
    },
    {
        "Type",
        "Tipo",
        "Type",
        "Typ",
        "Tipo",
        "Тип",
        "유형",
        "类型",
        "類型"
    },
    {
        "FILE",
        "ARCHIVO",
        "FICHIER",
        "DATEI",
        "FILE",
        "ФАЙЛ",
        "파일",
        "文件",
        "檔案"
    },
    {
        "Clone (1:1 Exact)",
        "Clonar (1:1 Exacto)",
        "Cloner (1:1 Exact)",
        "Klonen (1:1 Exakt)",
        "Clona (1:1 Esatto)",
        "Клонировать (1:1 копия)",
        "복제 (1:1 원본 복사)",
        "克隆 (1:1 完整复制)",
        "複製 (1:1 完整拷貝)"
    },
    {
        "Clone file name",
        "Nombre del clon",
        "Nom du clone",
        "Name des Klons",
        "Nome del clone",
        "Имя клона",
        "복제 파일 이름",
        "克隆文件名称",
        "複製檔案名稱"
    },
    {
        "Clone (New Identity)",
        "Clonar (Nueva Identidad)",
        "Cloner (Nouvelle Identité)",
        "Klonen (Neue Identität)",
        "Clona (Nuova Identità)",
        "Клонировать (Новый UID)",
        "복제 (새 고유 ID 생성)",
        "克隆 (生成全新身份 UID)",
        "複製 (產生全新身份 UID)"
    },
    {
        "New Identity clone name",
        "Nombre del clon",
        "Nom du clone",
        "Name des Klons",
        "Nome del clone",
        "Имя клона",
        "새 ID 복제 이름",
        "新身份克隆文件名",
        "新身份複製檔案名稱"
    },
    {
        "Edit Artwork (Image Editor)",
        "Editar Imagen (Editor)",
        "Modifier l'Image (Éditeur)",
        "Bild bearbeiten (Editor)",
        "Modifica Immagine (Editor)",
        "Изменить обложку (Редактор)",
        "이미지 편집 (편집기)",
        "编辑封面图像",
        "編輯封面圖片"
    },
    {
        "Copy",
        "Copiar",
        "Copier",
        "Kopieren",
        "Copia",
        "Копировать",
        "복사",
        "复制",
        "複製"
    },
    {
        "Cut",
        "Cortar",
        "Couper",
        "Ausschneiden",
        "Taglia",
        "Вырезать",
        "잘라내기",
        "剪切",
        "剪下"
    },
    {
        "Rename",
        "Renombrar",
        "Renommer",
        "Umbenennen",
        "Rinomina",
        "Переименовать",
        "이름 바꾸기",
        "重命名",
        "重新命名"
    },
    {
        "New name",
        "Nuevo nombre",
        "Nouveau nom",
        "Neuer Name",
        "Nuovo nome",
        "Новое имя",
        "새 이름",
        "新名称",
        "新名稱"
    },
    {
        "Delete",
        "Eliminar",
        "Supprimer",
        "Löschen",
        "Elimina",
        "Удалить",
        "삭제",
        "删除",
        "刪除"
    },
    {
        "CONFIRM DELETION",
        "CONFIRMAR ELIMINACIÓN",
        "CONFIRMER LA SUPPRESSION",
        "LÖSCHEN BESTÄTIGEN",
        "CONFERMA ELIMINAZIONE",
        "ПОДТВЕРЖДЕНИЕ УДАЛЕНИЯ",
        "삭제 확인",
        "确认删除",
        "確認刪除"
    },
    {
        "Are you sure you want to delete this?",
        "¿Estás seguro de que deseas eliminar esto?",
        "Voulez-vous vraiment supprimer cet élément ?",
        "Möchtest du dieses Element wirklich löschen?",
        "Sei sicuro di voler eliminare questo elemento?",
        "Вы уверены, что хотите удалить этот файл?",
        "정말로 이 항목을 삭제하시겠습니까?",
        "确定要删除此项目吗？",
        "確定要刪除此項目嗎？"
    },
    {
        "Yes, Delete",
        "Sí, Eliminar",
        "Oui, Supprimer",
        "Ja, Löschen",
        "Sì, Elimina",
        "Да, удалить",
        "예, 삭제",
        "确认删除",
        "確認刪除"
    },
    {
        "SYSTEM OPERATION",
        "OPERACIÓN DEL SISTEMA",
        "OPÉRATION SYSTÈME",
        "SYSTEMVORGANG",
        "OPERAZIONE DI SISTEMA",
        "СИСТЕМНАЯ ОПЕРАЦИЯ",
        "시스템 작업 중",
        "系统操作中",
        "系統操作中"
    },
    {
        "Deleting: %s",
        "Eliminando: %s",
        "Suppression : %s",
        "Lösche: %s",
        "Eliminazione: %s",
        "Удаление: %s",
        "삭제 중: %s",
        "正在删除：%s",
        "正在刪除：%s"
    },
    {
        "Processing storage...",
        "Procesando almacenamiento...",
        "Traitement du stockage...",
        "Verarbeite Speicher...",
        "Elaborazione memoria...",
        "Обработка хранилища...",
        "저장소 처리 중...",
        "正在处理存储...",
        "正在處理儲存空間..."
    },
    {
        "CREATION CRYSTAL / IMAGINATOR WARNING",
        "AVISO: CRISTAL DE CREACIÓN / IMAGINATOR",
        "AVERTISSEMENT : CRISTAL / IMAGINATOR",
        "WARNUNG: KREATIONSKRISTALL / IMAGINATOR",
        "AVVISO: CRISTALLO CREAZIONE / IMAGINATOR",
        "ВНИМАНИЕ: КРИСТАЛЛ ТВОРЕНИЯ / IMAGINATOR",
        "경고: 크리에이션 크리스탈 / 이매지네이터",
        "警告：创造水晶 / Imaginator 角色",
        "警告：創造水晶 / Imaginator 角色"
    },
    {
        "Creation Crystals and Imaginators permanently lock their Battle Class upon creation in Skylanders Imaginators.",
        "Los Cristales de Creación bloquean permanentemente su Clase de Batalla al crearse en Skylanders Imaginators.",
        "Les Cristaux de Création verrouillent définitivement leur Classe de Combat dans Skylanders Imaginators.",
        "Kreationskristalle sperren ihre Kampfklasse dauerhaft nach der Erstellung in Skylanders Imaginators.",
        "I Cristalli Creazione bloccano in modo permanente la Classe di Battaglia in Skylanders Imaginators.",
        "Кристаллы Творения навсегда блокируют боевой класс после создания в Skylanders Imaginators.",
        "크리에이션 크리스탈은 스카이랜더 이매지네이터에서 생성 시 전투 클래스가 영구 고정됩니다.",
        "创造水晶与 Imaginator 在游戏中创建后会永久锁定其战斗职业。",
        "創造水晶與 Imaginator 在遊戲中建立後會永久鎖定其戰鬥職業。"
    },
    {
        "Cloning with a New Identity generates a distinct UID/Serial, but the figure's locked class and character data will carry over unless reset using specialized tools.",
        "Clonar con Nueva Identidad genera un UID distinto, pero la clase bloqueada y los datos se mantendrán a menos que se reinicien con herramientas externas.",
        "Le clonage avec Nouvelle Identité génère un nouvel UID, mais la classe verrouillée et les données seront conservées.",
        "Das Klonen mit neuer Identität erzeugt eine neue UID, aber die gesperrte Klasse und Charakterdaten bleiben erhalten.",
        "La clonazione con Nuova Identità genera un nuovo UID, ma la classe bloccata e i dati del personaggio verranno mantenuti.",
        "Клонирование с новым UID создает новый серийный номер, но заблокированный класс и данные сохранятся.",
        "새 ID로 복제하면 고유 UID가 생성되지만 고정된 클래스와 데이터는 그대로 유지됩니다.",
        "使用新身份克隆会生成全新 UID，但已锁定的职业与角色数据仍会保留。",
        "使用新身份複製會產生全新 UID，但已鎖定的職業與角色資料仍會保留。"
    },
    {
        "Target",
        "Objetivo",
        "Cible",
        "Ziel",
        "Destinazione",
        "Цель",
        "대상",
        "目标",
        "目標"
    },
    {
        "Proceed with Clone",
        "Continuar con Clon",
        "Continuer le Clonage",
        "Klonen fortsetzen",
        "Procedi con il Clone",
        "Продолжить",
        "복제 진행",
        "继续克隆",
        "繼續複製"
    },
    {
        "New Identity clone name (.dump)",
        "Nombre del clon (.dump)",
        "Nom du clone (.dump)",
        "Name des Klons (.dump)",
        "Nome del clone (.dump)",
        "Имя клона (.dump)",
        "새 복제 파일 이름 (.dump)",
        "新身份克隆文件名 (.dump)",
        "新身份複製檔案名稱 (.dump)"
    },

    // --- Screen 2: Figure Image Editor ---
    {
        "Step 1/3: Select Figure Dump",
        "Paso 1/3: Seleccionar Figura",
        "Étape 1/3 : Choisir la Figurine",
        "Schritt 1/3: Figur auswählen",
        "Passo 1/3: Seleziona Statuetta",
        "Шаг 1/3: Выберите дамп фигурки",
        "1/3단계: 피규어 덤프 선택",
        "第 1/3 步：选择角色数据 (.dump)",
        "第 1/3 步：選擇角色資料 (.dump)"
    },
    {
        "Choose a figure (.dump) to assign or update custom artwork.",
        "Elige una figura (.dump) para asignar o actualizar su imagen.",
        "Choisissez une figurine (.dump) pour modifier son image.",
        "Wähle eine Figur (.dump), um ein eigenes Bild zuzuweisen.",
        "Scegli una statuetta (.dump) per assegnare o modificare l'immagine.",
        "Выберите фигурку (.dump) для установки или замены обложки.",
        "커스텀 이미지를 지정할 피규어(.dump)를 선택하세요.",
        "选择一个角色 (.dump) 以分配或更新自定义封面。",
        "選擇一個角色 (.dump) 以指定或更新自訂封面。"
    },
    {
        "Folder: %s",
        "Carpeta: %s",
        "Dossier : %s",
        "Ordner: %s",
        "Cartella: %s",
        "Папка: %s",
        "폴더: %s",
        "目录：%s",
        "目錄：%s"
    },
    {
        "[L] Figures",
        "[L] Figuras",
        "[L] Figurines",
        "[L] Figuren",
        "[L] Statuette",
        "[L] Фигурки",
        "[L] 피규어",
        "[L] 角色库",
        "[L] 角色庫"
    },
    {
        "[R] Scans",
        "[R] Scans",
        "[R] Scans",
        "[R] Scans",
        "[R] Scansioni",
        "[R] Сканы",
        "[R] 스캔본",
        "[R] 扫描目录",
        "[R] 掃描目錄"
    },
    {
        "(B) Menu",
        "(B) Menú",
        "(B) Menu",
        "(B) Menü",
        "(B) Menu",
        "(B) Меню",
        "(B) 메뉴",
        "(B) 菜单",
        "(B) 選單"
    },
    {
        "No favorites found in %s",
        "No se encontraron favoritos en %s",
        "Aucun favori trouvé dans %s",
        "Keine Favoriten gefunden in %s",
        "Nessun preferito trovato in %s",
        "Избранное не найдено в %s",
        "%s에 즐겨찾기가 없습니다",
        "在 %s 中未找到收藏角色",
        "在 %s 中找不到最愛角色"
    },
    {
        "Open the Tesla Overlay (L + D-Pad Down) while playing or in-game, and star figures to add them to your Favorites!",
        "¡Abre el Overlay de Tesla (L + D-Pad Abajo) en el juego y marca figuras con estrella para añadirlas a Favoritos!",
        "Ouvrez l'Overlay Tesla (L + Bas) en jeu et marquez des figurines d'une étoile pour les ajouter aux Favoris !",
        "Öffne das Tesla-Overlay (L + Steuerkreuz Unten) im Spiel und markiere Figuren als Favoriten!",
        "Apri il Tesla Overlay (L + Giù) nel gioco e aggiungi una stella alle statuette per salvarle nei Preferiti!",
        "Откройте оверлей Tesla (L + Вниз) в игре и отметьте фигурки звездочкой, чтобы добавить их в Избранное!",
        "게임 중 Tesla 오버레이(L + 십자키 아래)를 열고 별표로 즐겨찾기에 추가하세요!",
        "在游戏中打开 Tesla 悬浮窗（L + 方向键下），标星角色即可将其加入收藏夹！",
        "在遊戲中開啟 Tesla 懸浮窗（L + 方向鍵下），標星角色即可將其加入最愛！"
    },
    {
        "Figure / File",
        "Figura / Archivo",
        "Figurine / Fichier",
        "Figur / Datei",
        "Statuetta / File",
        "Фигурка / Файл",
        "피규어 / 파일",
        "角色 / 文件",
        "角色 / 檔案"
    },
    {
        "★ FAVORITE",
        "★ FAVORITO",
        "★ FAVORI",
        "★ FAVORIT",
        "★ PREFERITO",
        "★ ИЗБРАННОЕ",
        "★ 즐겨찾기",
        "★ 收藏",
        "★ 最愛"
    },
    {
        "Step 2/3: Choose Source Photo",
        "Paso 2/3: Elegir Foto de Origen",
        "Étape 2/3 : Choisir la Photo Source",
        "Schritt 2/3: Quellfoto auswählen",
        "Passo 2/3: Scegli Foto Sorgente",
        "Шаг 2/3: Выберите исходное фото",
        "2/3단계: 원본 사진 선택",
        "第 2/3 步：选择源图片 / 截图",
        "第 2/3 步：選擇來源圖片 / 截圖"
    },
    {
        "Assigning artwork for: %s",
        "Asignando imagen para: %s",
        "Attribution d'image pour : %s",
        "Bild zuweisen für: %s",
        "Immagine per: %s",
        "Обложка для: %s",
        "대상 피규어: %s",
        "目标角色：%s",
        "目標角色：%s"
    },
    {
        "[Y] Latest Photos",
        "[Y] Recientes",
        "[Y] Récentes",
        "[Y] Neueste Fotos",
        "[Y] Recenti",
        "[Y] Недавние",
        "[Y] 최근 사진",
        "[Y] 最新截图",
        "[Y] 最新截圖"
    },
    {
        "[X] Album Root",
        "[X] Álbum",
        "[X] Album",
        "[X] Album",
        "[X] Album",
        "[X] Альбом",
        "[X] 앨범 루트",
        "[X] 相册根目录",
        "[X] 相簿根目錄"
    },
    {
        "[L] SD Root (/)",
        "[L] Raíz SD (/)",
        "[L] Racine SD (/)",
        "[L] SD-Root (/)",
        "[L] Radice SD (/)",
        "[L] Корень SD (/)",
        "[L] SD 루트 (/)",
        "[L] SD卡根目录 (/)",
        "[L] SD卡根目錄 (/)"
    },
    {
        "(B) Back",
        "(B) Volver",
        "(B) Retour",
        "(B) Zurück",
        "(B) Indietro",
        "(B) Назад",
        "(B) 뒤로",
        "(B) 返回",
        "(B) 返回"
    },
    {
        "No photos found in this folder (%s)",
        "No se encontraron fotos en esta carpeta (%s)",
        "Aucune photo trouvée dans ce dossier (%s)",
        "Keine Fotos in diesem Ordner gefunden (%s)",
        "Nessuna foto trovata in questa cartella (%s)",
        "Фотографии в этой папке не найдены (%s)",
        "이 폴더에 사진이 없습니다 (%s)",
        "此目录中未找到图片 (%s)",
        "此目錄中找不到圖片 (%s)"
    },
    {
        "Switch screenshots are located in Album subfolders: YYYY/MM/DD/*.jpg.",
        "Las capturas de Switch están en subcarpetas del Álbum: AAAA/MM/DD/*.jpg.",
        "Les captures Switch sont dans les sous-dossiers Album : AAAA/MM/JJ/*.jpg.",
        "Switch-Screenshots liegen in Album-Unterordnern: JJJJ/MM/TT/*.jpg.",
        "Gli screenshot di Switch si trovano nelle sottocartelle Album: AAAA/MM/GG/*.jpg.",
        "Скриншоты Switch находятся в подпапках Альбома: ГГГГ/ММ/ДД/*.jpg.",
        "스위치 스크린샷은 앨범 하위 폴더(YYYY/MM/DD/*.jpg)에 있습니다.",
        "Switch 截图保存在相册的 年/月/日 (YYYY/MM/DD/*.jpg) 子目录中。",
        "Switch 截圖儲存在相簿的 年/月/日 (YYYY/MM/DD/*.jpg) 子目錄中。"
    },
    {
        "Navigate into the year/month/day folders above to select your screenshot, or use photos (.jpg / .png) placed on your SD card.",
        "Navega por las carpetas de año/mes/día arriba para elegir tu captura, o usa fotos (.jpg / .png) en tu tarjeta SD.",
        "Naviguez dans les dossiers année/mois/jour ci-dessus ou utilisez des photos (.jpg / .png) sur votre carte SD.",
        "Navigiere oben durch die Jahr/Monat/Tag-Ordner oder nutze eigene Fotos (.jpg / .png) auf der SD-Karte.",
        "Naviga nelle cartelle anno/mese/giorno qui sopra o usa foto (.jpg / .png) sulla scheda SD.",
        "Перейдите в папки год/месяц/день выше или выберите любое фото (.jpg / .png) на SD-карте.",
        "위의 연/월/일 폴더로 이동하여 스크린샷을 선택하거나 SD 카드의 이미지를 선택하세요.",
        "请进入上方的年/月/日文件夹选择截图，或选择 SD 卡上的任意图片 (.jpg / .png)。",
        "請進入上方的年/月/日資料夾選擇截圖，或選擇 SD 卡上的任意圖片 (.jpg / .png)。"
    },
    {
        "Preview / Name",
        "Vista Previa / Nombre",
        "Aperçu / Nom",
        "Vorschau / Name",
        "Anteprima / Nome",
        "Превью / Имя",
        "미리보기 / 이름",
        "预览 / 文件名",
        "預覽 / 檔案名稱"
    },
    {
        "Step 3/3: Crop & Save Artwork",
        "Paso 3/3: Recortar y Guardar Arte",
        "Étape 3/3 : Recadrer et Sauvegarder",
        "Schritt 3/3: Zuschneiden & Speichern",
        "Passo 3/3: Ritaglia e Salva",
        "Шаг 3/3: Кадрирование и сохранение",
        "3/3단계: 자르기 및 저장",
        "第 3/3 步：裁剪并保存封面",
        "第 3/3 步：裁切並儲存封面"
    },
    {
        "Target: %s  ->  %s",
        "Objetivo: %s  ->  %s",
        "Cible : %s  ->  %s",
        "Ziel: %s  ->  %s",
        " File: %s  ->  %s",
        "Цель: %s  ->  %s",
        "대상: %s  ->  %s",
        "目标：%s  ->  %s",
        "目標：%s  ->  %s"
    },
    {
        "Live Preview (175 x 275)",
        "Vista Previa (175 x 275)",
        "Aperçu (175 x 275)",
        "Vorschau (175 x 275)",
        "Anteprima (175 x 275)",
        "Превью (175 x 275)",
        "미리보기 (175 x 275)",
        "实时预览 (175 x 275)",
        "即時預覽 (175 x 275)"
    },
    {
        "Save Artwork (A)",
        "Guardar Arte (A)",
        "Sauvegarder (A)",
        "Speichern (A)",
        "Salva Immagine (A)",
        "Сохранить (A)",
        "이미지 저장 (A)",
        "保存封面 (A)",
        "儲存封面 (A)"
    },
    {
        "Reset Frame (X)",
        "Reiniciar (X)",
        "Réinitialiser (X)",
        "Zurücksetzen (X)",
        "Reimposta (X)",
        "Сбросить (X)",
        "초기화 (X)",
        "重置选框 (X)",
        "重置選框 (X)"
    },
    {
        "Back to Photos (B)",
        "Volver (B)",
        "Retour (B)",
        "Zurück (B)",
        "Indietro (B)",
        "Назад (B)",
        "뒤로 (B)",
        "返回照片 (B)",
        "返回照片 (B)"
    },

    // --- Screen 3 & 4: Downloaders ---
    {
        "Download .dump files from a remote ZIP URL.",
        "Descargar archivos .dump desde una URL de archivo ZIP remoto.",
        "Télécharger des fichiers .dump depuis une URL ZIP distante.",
        "Lade .dump-Dateien von einer externen ZIP-URL herunter.",
        "Scarica file .dump da un URL ZIP remoto.",
        "Загрузка файлов .dump по прямой ссылке на ZIP-архив.",
        "원격 ZIP URL에서 .dump 파일을 다운로드합니다.",
        "从远程 ZIP 链接下载 .dump 角色数据文件。",
        "從遠端 ZIP 連結下載 .dump 角色資料檔案。"
    },
    {
        "Custom URL",
        "URL Personalizada",
        "URL Personnalisée",
        "Eigene URL",
        "URL Personalizzato",
        "Ссылка URL",
        "사용자 URL",
        "自定义链接",
        "自訂連結"
    },
    {
        "(L) Edit URL",
        "(L) Editar URL",
        "(L) Modifier URL",
        "(L) URL ändern",
        "(L) Modifica URL",
        "(L) Изм. URL",
        "(L) URL 편집",
        "(L) 编辑链接",
        "(L) 編輯連結"
    },
    {
        "Enter ZIP URL",
        "Introduce la URL del ZIP",
        "Entrez l'URL du ZIP",
        "ZIP-URL eingeben",
        "Inserisci URL ZIP",
        "Введите ссылку на ZIP",
        "ZIP URL 입력",
        "输入 ZIP 下载链接",
        "輸入 ZIP 下載連結"
    },
    {
        "Download & Extract",
        "Descargar y Extraer",
        "Télécharger et Extraire",
        "Herunterladen & Entpacken",
        "Scarica ed Estrai",
        "Скачать и распаковать",
        "다운로드 및 압축 해제",
        "下载并解压",
        "下載並解壓縮"
    },
    {
        "Downloading...",
        "Descargando...",
        "Téléchargement...",
        "Lade herunter...",
        "Download in corso...",
        "Загрузка...",
        "다운로드 중...",
        "正在下载...",
        "正在下載..."
    },
    {
        "Download failed! Check your internet connection or URL.",
        "¡Error en la descarga! Verifica tu conexión o la URL.",
        "Échec du téléchargement ! Vérifiez votre connexion ou l'URL.",
        "Download fehlgeschlagen! Prüfe deine Verbindung oder URL.",
        "Download fallito! Controlla la connessione o l'URL.",
        "Ошибка загрузки! Проверьте подключение к сети или URL.",
        "다운로드 실패! 인터넷 연결이나 URL을 확인하세요.",
        "下载失败！请检查网络连接或 URL 链接。",
        "下載失敗！請檢查網路連線或 URL 連結。"
    },
    {
        "Extracting...",
        "Extrayendo...",
        "Extraction...",
        "Entpacke...",
        "Estrazione...",
        "Распаковка...",
        "압축 해제 중...",
        "正在解压...",
        "正在解壓縮..."
    },
    {
        "Download finished! Files saved to sdmc:/emulanders/figures/Downloads/",
        "¡Descarga completada! Archivos en sdmc:/emulanders/figures/Downloads/",
        "Téléchargement terminé ! Fichiers dans sdmc:/emulanders/figures/Downloads/",
        "Download fertig! Gespeichert in sdmc:/emulanders/figures/Downloads/",
        "Download completato! File in sdmc:/emulanders/figures/Downloads/",
        "Загрузка завершена! Файлы в sdmc:/emulanders/figures/Downloads/",
        "다운로드 완료! sdmc:/emulanders/figures/Downloads/에 저장됨",
        "下载完成！文件已保存至 sdmc:/emulanders/figures/Downloads/",
        "下載完成！檔案已儲存至 sdmc:/emulanders/figures/Downloads/"
    },
    {
        "Processing...",
        "Procesando...",
        "Traitement...",
        "Verarbeite...",
        "Elaborazione...",
        "Обработка...",
        "처리 중...",
        "正在处理...",
        "正在處理..."
    },
    {
        "%.1f MB processed",
        "%.1f MB procesados",
        "%.1f Mo traités",
        "%.1f MB verarbeitet",
        "%.1f MB elaborati",
        "Обработано %.1f МБ",
        "%.1f MB 처리됨",
        "已处理 %.1f MB",
        "已處理 %.1f MB"
    },
    {
        "Detected URLs in sdmc:/emulanders/dumps_urls.txt:",
        "URLs detectadas en sdmc:/emulanders/dumps_urls.txt:",
        "URLs détectées dans sdmc:/emulanders/dumps_urls.txt :",
        "Erkannte URLs in sdmc:/emulanders/dumps_urls.txt:",
        "URL rilevati in sdmc:/emulanders/dumps_urls.txt:",
        "Найденные ссылки в sdmc:/emulanders/dumps_urls.txt:",
        "sdmc:/emulanders/dumps_urls.txt에서 감지된 URL:",
        "在 sdmc:/emulanders/dumps_urls.txt 中检测到的链接：",
        "在 sdmc:/emulanders/dumps_urls.txt 中偵測到的連結："
    },
    {
        "Use",
        "Usar",
        "Utiliser",
        "Nutzen",
        "Usa",
        "Выбрать",
        "사용",
        "使用",
        "使用"
    },
    {
        "Download the community asset pack (figure images for all Skylanders).",
        "Descargar el paquete de imágenes de la comunidad (fotos de todos los Skylanders).",
        "Télécharger le pack d'images communautaire (illustrations de tous les Skylanders).",
        "Lade das Community-Bilderpaket herunter (Figurenbilder für alle Skylanders).",
        "Scarica il pacchetto immagini della community (foto di tutti gli Skylanders).",
        "Скачать пакет изображений сообщества (обложки для всех фигурок Skylanders).",
        "커뮤니티 이미지 팩(모든 스카이랜더 피규어 사진)을 다운로드합니다.",
        "下载社区角色图包（包含所有 Skylanders 角色的高清预览图）。",
        "下載社群角色圖包（包含所有 Skylanders 角色的高清預覽圖）。"
    },
    {
        "This is recommended for the best experience with the scanner and file manager.",
        "Recomendado para obtener la mejor experiencia con el lector y el gestor de archivos.",
        "Recommandé pour une expérience optimale avec le lecteur et l'overlay.",
        "Empfohlen für die beste Darstellung im Scanner, Dateimanager und Overlay.",
        "Consigliato per la migliore esperienza con lo scanner, il gestore file e l'overlay.",
        "Рекомендуется для отображения картинок фигурок в сканере, менеджере и оверлее.",
        "스캐너, 파일 관리자 및 오버레이에서 최상의 환경을 위해 권장됩니다.",
        "推荐安装，以便在读取器、文件管理器和 Tesla 悬浮窗中显示角色图片。",
        "推薦安裝，以便在讀取器、檔案管理器和 Tesla 懸浮窗中顯示角色圖片。"
    },
    {
        "System Busy...",
        "Sistema Ocupado...",
        "Système Occupé...",
        "System Beschäftigt...",
        "Sistema Occupato...",
        "Система занята...",
        "시스템 작업 중...",
        "系统繁忙...",
        "系統忙碌中..."
    },
    {
        "Download Assets",
        "Descargar Assets",
        "Télécharger les Assets",
        "Assets Herunterladen",
        "Scarica Immagini",
        "Скачать обложки",
        "에셋 다운로드",
        "下载角色图包",
        "下載角色圖包"
    },
    {
        "Download failed! Check your internet connection.",
        "¡Error en la descarga! Verifica tu conexión a internet.",
        "Échec du téléchargement ! Vérifiez votre connexion internet.",
        "Download fehlgeschlagen! Prüfe deine Internetverbindung.",
        "Download fallito! Controlla la connessione internet.",
        "Ошибка загрузки! Проверьте подключение к интернету.",
        "다운로드 실패! 인터넷 연결을 확인하세요.",
        "下载失败！请检查您的网络连接。",
        "下載失敗！請檢查您的網路連線。"
    },
    {
        "Assets installed!",
        "¡Assets instalados con éxito!",
        "Assets installés avec succès !",
        "Assets erfolgreich installiert!",
        "Immagini installate con successo!",
        "Обложки успешно установлены!",
        "에셋 설치 완료!",
        "角色图包安装成功！",
        "角色圖包安裝成功！"
    },
    {
        "%.1f MB downloaded",
        "%.1f MB descargados",
        "%.1f Mo téléchargés",
        "%.1f MB heruntergeladen",
        "%.1f MB scaricati",
        "Скачано %.1f МБ",
        "%.1f MB 다운로드됨",
        "已下载 %.1f MB",
        "已下載 %.1f MB"
    },

    // --- Screen 5: Settings ---
    {
        "APPLICATION LANGUAGE",
        "IDIOMA DE LA APLICACIÓN",
        "LANGUE DE L'APPLICATION",
        "ANWENDUNGSSPRACHE",
        "LINGUA DELL'APPLICAZIONE",
        "ЯЗЫК ПРИЛОЖЕНИЯ",
        "애플리케이션 언어",
        "应用程序语言",
        "應用程式語言"
    },
    {
        "Select interface language (shared with Tesla Overlay):",
        "Selecciona el idioma de la interfaz (sincronizado con el Overlay):",
        "Choisissez la langue de l'interface (partagée avec l'Overlay Tesla) :",
        "Wähle die Sprache der Oberfläche (synchronisiert mit dem Tesla-Overlay):",
        "Seleziona la lingua dell'interfaccia (condivisa con il Tesla Overlay):",
        "Выберите язык интерфейса (синхронизируется с оверлеем Tesla):",
        "인터페이스 언어 선택 (Tesla 오버레이와 자동 동기화):",
        "选择界面语言（与 Tesla 悬浮窗实时同步）：",
        "選擇介面語言（與 Tesla 懸浮窗即時同步）："
    },
    {
        "THEME & APPEARANCE",
        "TEMA Y APARIENCIA",
        "THÈME ET APPARENCE",
        "DESIGN & DARSTELLUNG",
        "TEMA E ASPETTO",
        "ТЕМА И ОФОРМЛЕНИЕ",
        "테마 및 외관",
        "主题与外观",
        "主題與外觀"
    },
    {
        "Color Palette Presets:",
        "Ajustes Predefinidos de Color:",
        "Palettes de Couleurs Prédéfinies :",
        "Farbvoreinstellungen:",
        "Predefiniti Colore:",
        "Готовые цветовые схемы:",
        "색상 프리셋:",
        "预设主题配色：",
        "預設主題配色："
    },
    {
        "Cyan (Default)",
        "Cian (Por defecto)",
        "Cyan (Défaut)",
        "Cyan (Standard)",
        "Ciano (Predefinito)",
        "Голубой (По умолч.)",
        "시안 (기본)",
        "青色 (默认)",
        "青色 (預設)"
    },
    {
        "Portal Blue",
        "Azul Portal",
        "Bleu Portail",
        "Portal-Blau",
        "Blu Portale",
        "Синий портал",
        "포털 블루",
        "传送门蓝",
        "傳送門藍"
    },
    {
        "Neon Purple",
        "Púrpura Neón",
        "Violet Néon",
        "Neon-Lila",
        "Viola Neon",
        "Неоновый фиолетовый",
        "네온 퍼플",
        "霓虹紫",
        "霓虹紫"
    },
    {
        "Emerald",
        "Esmeralda",
        "Émeraude",
        "Smaragd",
        "Smeraldo",
        "Изумрудный",
        "에메랄드",
        "翡翠绿",
        "翡翠綠"
    },
    {
        "Amber Gold",
        "Oro Ámbar",
        "Or Ambré",
        "Bernstein-Gold",
        "Oro Ambra",
        "Янтарное золото",
        "앰버 골드",
        "琥珀金",
        "琥珀金"
    },
    {
        "Crimson",
        "Carmesí",
        "Cramoisi",
        "Karmesinrot",
        "Cremisi",
        "Малиновый",
        "크림슨 레드",
        "绯红色",
        "緋紅色"
    },
    {
        "Hot Pink",
        "Rosa Intenso",
        "Rose Vif",
        "Knallrosa",
        "Rosa Acceso",
        "Ярко-розовый",
        "핫 핑크",
        "亮粉色",
        "亮粉色"
    },
    {
        "Custom Accent Color:",
        "Color de Acento Personalizado:",
        "Couleur d'Accentuation Personnalisée :",
        "Eigene Akzentfarbe:",
        "Colore Principale Personalizzato:",
        "Пользовательский цвет акцента:",
        "사용자 지정 강조 색상:",
        "自定义强调色：",
        "自訂強調色："
    },
    {
        "Red##accent_r",
        "Rojo##accent_r",
        "Rouge##accent_r",
        "Rot##accent_r",
        "Rosso##accent_r",
        "Красный##accent_r",
        "빨강##accent_r",
        "红 (R)##accent_r",
        "紅 (R)##accent_r"
    },
    {
        "Green##accent_g",
        "Verde##accent_g",
        "Vert##accent_g",
        "Grün##accent_g",
        "Verde##accent_g",
        "Зеленый##accent_g",
        "초록##accent_g",
        "绿 (G)##accent_g",
        "綠 (G)##accent_g"
    },
    {
        "Blue##accent_b",
        "Azul##accent_b",
        "Bleu##accent_b",
        "Blau##accent_b",
        "Blu##accent_b",
        "Синий##accent_b",
        "파랑##accent_b",
        "蓝 (B)##accent_b",
        "藍 (B)##accent_b"
    },
    {
        "Click or press (A) to open color palette",
        "Haz clic o pulsa (A) para abrir la paleta de colores",
        "Cliquez ou appuyez sur (A) pour ouvrir la palette",
        "Klicken oder (A) drücken, um die Farbpalette zu öffnen",
        "Clicca o premi (A) per aprire la tavolozza colori",
        "Нажмите (A), чтобы открыть палитру цветов",
        "클릭하거나 (A)를 눌러 색상 팔레트 열기",
        "点击或按 (A) 打开调色盘",
        "點擊或按 (A) 開啟調色盤"
    },
    {
        "Color Palette",
        "Paleta de Colores",
        "Palette de Couleurs",
        "Farbpalette",
        "Tavolozza Colori",
        "Палитра цветов",
        "색상 팔레트",
        "调色盘",
        "調色盤"
    },
    {
        "Close (B)",
        "Cerrar (B)",
        "Fermer (B)",
        "Schließen (B)",
        "Chiudi (B)",
        "Закрыть (B)",
        "닫기 (B)",
        "关闭 (B)",
        "關閉 (B)"
    },
    {
        "Save Configuration",
        "Guardar Configuración",
        "Sauvegarder la Configuration",
        "Konfiguration Speichern",
        "Salva Configurazione",
        "Сохранить настройки",
        "설정 저장",
        "保存配置",
        "儲存設定"
    },
    {
        "Settings saved to sdmc:/emulanders/config.json",
        "Ajustes guardados en sdmc:/emulanders/config.json",
        "Paramètres sauvegardés dans sdmc:/emulanders/config.json",
        "Einstellungen gespeichert in sdmc:/emulanders/config.json",
        "Impostazioni salvate in sdmc:/emulanders/config.json",
        "Настройки сохранены в sdmc:/emulanders/config.json",
        "설정이 sdmc:/emulanders/config.json에 저장되었습니다",
        "设置已保存至 sdmc:/emulanders/config.json",
        "設定已儲存至 sdmc:/emulanders/config.json"
    },
    {
        "SYSTEM & BACKEND STATUS",
        "ESTADO DEL SISTEMA Y BACKEND",
        "ÉTAT DU SYSTÈME ET DU SERVICE",
        "SYSTEM- & BACKEND-STATUS",
        "STATO DEL SISTEMA E BACKEND",
        "СТАТУС СИСТЕМЫ И СЛУЖБЫ",
        "시스템 및 백엔드 상태",
        "系统与后台服务状态",
        "系統與背景服務狀態"
    },
    {
        "Sysmodule IPC: ",
        "IPC del Sysmodule: ",
        "IPC Sysmodule : ",
        "Sysmodule-IPC: ",
        "IPC Sysmodule: ",
        "Служба IPC: ",
        "시스모듈 IPC: ",
        "后台服务 IPC：",
        "系統模組 IPC："
    },
    {
        "Connected (emulanders)",
        "Conectado (emulanders)",
        "Connecté (emulanders)",
        "Verbunden (emulanders)",
        "Connesso (emulanders)",
        "Подключено (emulanders)",
        "연결됨 (emulanders)",
        "已连接 (emulanders)",
        "已連線 (emulanders)"
    },
    {
        "Not Running / Offline",
        "No Iniciado / Desconectado",
        "Arrêté / Hors ligne",
        "Nicht aktiv / Offline",
        "Non attivo / Offline",
        "Не запущено / Офлайн",
        "실행 안 됨 / 오프라인",
        "未运行 / 离线",
        "未執行 / 離線"
    },
    {
        "Emulation State: ",
        "Estado de Emulación: ",
        "État de l'Émulation : ",
        "Emulationsstatus: ",
        "Stato Emulazione: ",
        "Статус эмуляции: ",
        "에뮬레이션 상태: ",
        "模拟状态：",
        "模擬狀態："
    },
    {
        "Active (Emulating Virtual Portal)",
        "Activo (Emulando Portal Virtual)",
        "Actif (Émulation du Portail Virtuel)",
        "Aktiv (Virtuelles Portal aktiv)",
        "Attivo (Portale Virtuale Attivo)",
        "Активна (Виртуальный портал)",
        "활성 (가상 포털 에뮬레이션 중)",
        "已激活 (正在模拟虚拟传送门)",
        "已啟用 (正在模擬虛擬傳送門)"
    },
    {
        "Standby (Passthrough Mode)",
        "En Espera (Modo Passthrough)",
        "En Veille (Mode Passthrough)",
        "Standby (Passthrough-Modus)",
        "In Attesa (Modalità Passthrough)",
        "Ожидание (Режим пропуска)",
        "대기 중 (패스스루 모드)",
        "待机 (直通模式)",
        "待機 (直通模式)"
    },
    {
        "Active Figure: ",
        "Figura Activa: ",
        "Figurine Active : ",
        "Aktive Figur: ",
        "Statuetta Attiva: ",
        "Активная фигурка: ",
        "활성 피규어: ",
        "当前角色：",
        "目前角色："
    },
    {
        "None (Portal Empty)",
        "Ninguna (Portal Vacío)",
        "Aucune (Portail Vide)",
        "Keine (Portal leer)",
        "Nessuna (Portale Vuoto)",
        "Нет (Портал пуст)",
        "없음 (포털 비어 있음)",
        "无 (传送门为空)",
        "無 (傳送門為空)"
    },
    {
        "Figures Directory:  %s",
        "Carpeta de Figuras: %s",
        "Dossier Figurines : %s",
        "Figuren-Verzeichnis: %s",
        "Cartella Statuette: %s",
        "Папка фигурок:      %s",
        "피규어 디렉터리:    %s",
        "角色目录：          %s",
        "角色目錄：          %s"
    },
    {
        "Assets Directory:   sdmc:/emulanders/assets/",
        "Carpeta de Assets:  sdmc:/emulanders/assets/",
        "Dossier Assets :    sdmc:/emulanders/assets/",
        "Assets-Verzeichnis: sdmc:/emulanders/assets/",
        "Cartella Immagini:  sdmc:/emulanders/assets/",
        "Папка обложек:      sdmc:/emulanders/assets/",
        "에셋 디렉터리:      sdmc:/emulanders/assets/",
        "图包目录：          sdmc:/emulanders/assets/",
        "圖包目錄：          sdmc:/emulanders/assets/"
    },
    {
        "Config File:        sdmc:/emulanders/config.json",
        "Archivo de Config:  sdmc:/emulanders/config.json",
        "Fichier Config :    sdmc:/emulanders/config.json",
        "Konfigurationsdatei: sdmc:/emulanders/config.json",
        "File Config:        sdmc:/emulanders/config.json",
        "Файл настроек:      sdmc:/emulanders/config.json",
        "설정 파일:          sdmc:/emulanders/config.json",
        "配置文件：          sdmc:/emulanders/config.json",
        "設定檔案：          sdmc:/emulanders/config.json"
    },
    {
        "Language:           %s",
        "Idioma:             %s",
        "Langue :            %s",
        "Sprache:            %s",
        "Lingua:             %s",
        "Язык:               %s",
        "언어:               %s",
        "当前语言：          %s",
        "目前語言：          %s"
    }
};

const char* TranslateString(const char* en, const char* pt, AppLanguage lang) {
    if (lang == AppLanguage::EN) return en;
    if (lang == AppLanguage::PT_BR) return pt;

    for (const auto& entry : kTranslations) {
        if (strcmp(entry.en, en) == 0) {
            switch (lang) {
                case AppLanguage::ES:      return entry.es;
                case AppLanguage::FR:      return entry.fr;
                case AppLanguage::DE:      return entry.de;
                case AppLanguage::IT:      return entry.it;
                case AppLanguage::RU:      return entry.ru;
                case AppLanguage::KO:      return entry.ko;
                case AppLanguage::ZH_HANS: return entry.zh_hans;
                case AppLanguage::ZH_HANT: return entry.zh_hant;
                default: return en;
            }
        }
    }
    return en;
}

void BuildExtraLanguageGlyphRanges(ImFontGlyphRangesBuilder& builder) {
    for (const auto& opt : kSupportedLanguages) {
        builder.AddText(opt.display_name);
    }
    for (const auto& entry : kTranslations) {
        builder.AddText(entry.ko);
        builder.AddText(entry.zh_hans);
        builder.AddText(entry.zh_hant);
    }
}
