#include "smf_player.hpp"

#define DEBUG

SMFPlayer* gPlayer;

/// <summary>
/// SMFファイルをオープンする
/// </summary>
/// <param name="fileName">ファイル名</param>
/// <returns>0:成功/0以外:失敗</returns>
int SMFPlayer::Open(const char* fileName) {
	size_t len = 0;
	uint16_t format, trackCount;
	uint32_t magic, chunkLength;

#if defined _WIN32 || defined DEBUG
	printf("SMF File Open.\n");
#endif

	/* ファイルをオープンする*/
#if defined _WIN32
	smfFile = fopen(fileName, "rb");
	/* 開けなかったらエラー */
	if (smfFile == NULL) {
		return SMF_RET_ERR_FILE_NOT_FOUND;
	}
#else
	FRESULT res;
	FILINFO fno;
	res = f_open(&smfFile, fileName, FA_OPEN_EXISTING | FA_READ);
	if (res != FR_OK) return SMF_RET_ERR_FILE_NOT_FOUND;
#endif

	/* マジックワードをチェックする */
	if (get4(&magic) != SMF_RET_OK || magic != 0x4D546864) {
#if defined _WIN32
		fclose(smfFile);
#else
		f_close(&smfFile);
#endif
		return SMF_RET_ERR_WROND_MAGIC;
	}

	/* データ長さ取得 */
	if (get4(&chunkLength) != SMF_RET_OK || chunkLength != 6) {
#if defined _WIN32
		fclose(smfFile);
#else
		f_close(&smfFile);
#endif
		return SMF_RET_ERR_WRONG_LENGTH;
	}

	/* フォーマット取得 */
	if (get2(&format) != SMF_RET_OK || format != 0) {
#if defined _WIN32
		fclose(smfFile);
#else
		f_close(&smfFile);
#endif
		return SMF_RET_ERR_WRONG_FORMAT;
	}

	/* トラック数取得 */
	if (get2(&trackCount) != SMF_RET_OK || trackCount != 1) {
#if defined _WIN32
		fclose(smfFile);
#else
		f_close(&smfFile);
#endif
		return SMF_RET_ERR_WRONG_COUNT;
	}

	/* 分解能取得 */
	if (get2(&resolution) != SMF_RET_OK || resolution == 0 ||
		resolution & 0x8000) {
#if defined _WIN32
		fclose(smfFile);
#else
		f_close(&smfFile);
#endif
		return SMF_RET_ERR_WRONG_RESOLUTION;
	}

#if defined _WIN32 || defined DEBUG
	printf("\tFILENAME     :%s\n", fileName);
	printf("\tMAGIC        :%08Xh\n", magic);
	printf("\tCHUNK LENGTH :%08Xh\n", chunkLength);
	printf("\tFORMAT       :%04Xh\n", format);
	printf("\tTRACK COUNT  :%04Xh\n", trackCount);
	printf("\tRESOLUTION   :%d\n", resolution);
	printf("OK!\n");
#endif

	/* トラックに移動する */
	if (get4(&magic) != SMF_RET_OK || magic != 0x4D54726B) {
		return SMF_RET_ERR_TRACK_NOT_FOUND;
	}

	/* データ長さ取得 */
	if (get4(&chunkLength) != SMF_RET_OK || chunkLength == 0) {
#if defined _WIN32
		fclose(smfFile);
#else
		f_close(&smfFile);
#endif
		return SMF_RET_ERR_WRONG_LENGTH;
	}

#if defined _WIN32 || defined DEBUG
	printf("Track Chunk Open.\n");
	printf("\tMAGIC        :%08Xh\n", magic);
	printf("\tCHUNK LENGTH :%08Xh\n", chunkLength);
	printf("OK!\n");
#endif
	getVarLen(&delta);
#if defined _WIN32 || defined DEBUG
	printf("\tFIRST DELTA  :%08x\n", delta);
#endif
	return SMF_RET_OK;
	}

int SMFPlayer::Play(uint64_t* nextDelta) {
	size_t size;
	uint64_t d;
	do {
		if (getMessage(message, &size) != SMF_RET_OK)
			return SMF_RET_ERR_REACHED_EOF;
		if (getVarLen(&delta) != SMF_RET_OK) return SMF_RET_ERR_REACHED_EOF;
		midiMessageCallback(message, size);
		d = delta * deltaBase;
	} while (delta == 0);
	*nextDelta = d;
	return SMF_RET_OK;
}

int SMFPlayer::Close(void) {
	const uint8_t cRESET_GM[] = { 0xF0, 0x7E, 0x7F, 0x09, 0x01, 0xF7 };
	midiMessageCallback((uint8_t*)cRESET_GM, sizeof(cRESET_GM));
#if defined _WIN32
	fclose(smfFile);
#else
	f_close(&smfFile);
#endif
	return SMF_RET_OK;
}

int SMFPlayer::get1(uint8_t* dst) {
	size_t len = 0;
#if defined _WIN32
	len = fread(dst, sizeof(uint8_t), 1, smfFile);
	if (len == 1) return SMF_RET_OK;
#else
	FRESULT res;
	UINT br;
	res = f_read(&smfFile, dst, 1, &br);
	if (br == 1) return SMF_RET_OK;
#endif
	return SMF_RET_ERR_REACHED_EOF;
}

int SMFPlayer::get2(uint16_t* dst) {
	uint8_t tmp = 0;
	size_t len = 0;
	*dst = 0;
	if (get1(&tmp) != SMF_RET_OK) return SMF_RET_ERR_REACHED_EOF;
	*dst = (uint16_t)tmp << 8;
	if (get1(&tmp) != SMF_RET_OK) return SMF_RET_ERR_REACHED_EOF;
	*dst |= (uint16_t)tmp;
	return SMF_RET_OK;
}

int SMFPlayer::get4(uint32_t* dst) {
	uint8_t tmp = 0;
	size_t len = 0;
	*dst = 0;
	if (get1(&tmp) != SMF_RET_OK) return SMF_RET_ERR_REACHED_EOF;
	*dst = (uint32_t)tmp << 24;
	if (get1(&tmp) != SMF_RET_OK) return SMF_RET_ERR_REACHED_EOF;
	*dst |= (uint32_t)tmp << 16;
	if (get1(&tmp) != SMF_RET_OK) return SMF_RET_ERR_REACHED_EOF;
	*dst |= (uint32_t)tmp << 8;
	if (get1(&tmp) != SMF_RET_OK) return SMF_RET_ERR_REACHED_EOF;
	*dst |= (uint32_t)tmp;
	return SMF_RET_OK;
}

inline int SMFPlayer::getVarLen(uint32_t* dst) {
	uint8_t tmp = 0;
	*dst = 0;
	do {
		if (get1(&tmp) != SMF_RET_OK) return SMF_RET_ERR_REACHED_EOF;
		*dst = (*dst << 7) | (tmp & 0x7F);
	} while (tmp & 0x80);
	return 0;
}

inline int SMFPlayer::getMessage(uint8_t* dst, size_t* size) {
	uint8_t* initial = dst;
	uint8_t tmp, op, type;
	uint32_t length;

	int len = 0;
	if (get1(&tmp) != SMF_RET_OK) return SMF_RET_ERR_REACHED_EOF;
	if (tmp & 0xF0) {
		/* ステータス */
		status = tmp;
	}

	*dst++ = status;

	if (status == MidiMessageType_BeginSystemExclusive) {
		/* Length,Data... */
		if (getVarLen(&length) != SMF_RET_OK) return SMF_RET_ERR_REACHED_EOF;
		for (int i = 0; i < length; i++) {
			if (get1(dst++) != SMF_RET_OK) return SMF_RET_ERR_REACHED_EOF;
		}
	}
	else if (status == MidiMessageType_MetaEvent) {
		/* Type,Length,Data... */
		if (get1(&type) != SMF_RET_OK) return SMF_RET_ERR_REACHED_EOF;
		if (getVarLen(&length) != SMF_RET_OK) return SMF_RET_ERR_REACHED_EOF;
		for (int i = 0; i < length; i++) {
			if (get1(&meta[i]) != SMF_RET_OK) return SMF_RET_ERR_REACHED_EOF;
		}

		switch (type) {
		case 0x51:
			/* セットテンポ */
			tempo = (meta[0] << 16) | (meta[1] << 8) | (meta[2]);
			deltaBase = tempo / resolution;
#if defined _WIN32
			printf("SET TEMPO:%d,DELTA BASE:%d\n", tempo, deltaBase);
#endif
			break;
		default:
			break;
		}
	}
	else {
		/* 長さを推定 */
		op = tmp & 0xF0;
		switch (op) {
		case MidiMessageType_NoteOff:
		case MidiMessageType_NoteOn:
		case MidiMessageType_PolyphonicKeyPressure:
		case MidiMessageType_ControlChange:
		case MidiMessageType_PitchBend:
			length = 2;
			break;
		default:
			length = 1;
		}

		for (int i = 0; i < length; i++) {
			if (get1(dst++) != SMF_RET_OK) return SMF_RET_ERR_REACHED_EOF;
		}
	}
	*size = (size_t)(dst - initial);
	return SMF_RET_OK;
}
