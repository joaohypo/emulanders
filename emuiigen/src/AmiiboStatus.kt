package com.xortroll.emuiibo.emuiigen

import java.util.*

enum class AmiiboStatusKind {
    Ok,
    FlagNotFound,
    JsonNotFound,
    InvalidUuidLength,
    MiiCharInfoNotFound,
    InvalidNameLength;

    infix fun and(other: AmiiboStatusKind) = AmiiboStatus.of(this, other)
}

typealias AmiiboStatus = EnumSet<AmiiboStatusKind>

infix fun AmiiboStatus.and(other: AmiiboStatusKind) = AmiiboStatus.of(other, *this.toTypedArray())