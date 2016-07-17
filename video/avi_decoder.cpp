/* Cabal - Legacy Game Implementations
 *
 * Cabal is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

// Based on the ScummVM (GPLv2+) file of the same name

#include "common/stream.h"
#include "common/system.h"
#include "common/textconsole.h"

#include "audio/audiostream.h"
#include "audio/mixer.h"

#include "video/avi_decoder.h"

// Audio Codecs
#include "audio/decoders/adpcm.h"
#include "audio/decoders/mp3.h"
#include "audio/decoders/pcm.h"
#include "audio/decoders/vorbis.h"

// Video Codecs
#include "image/codecs/codec.h"

namespace Video {

#define UNKNOWN_HEADER(a) error("Unknown header found -- \'%s\'", tag2str(a))

// IDs used throughout the AVI files
// that will be handled by this player
#define ID_RIFF MKTAG('R','I','F','F')
#define ID_AVI  MKTAG('A','V','I',' ')
#define ID_LIST MKTAG('L','I','S','T')
#define ID_HDRL MKTAG('h','d','r','l')
#define ID_AVIH MKTAG('a','v','i','h')
#define ID_STRL MKTAG('s','t','r','l')
#define ID_STRH MKTAG('s','t','r','h')
#define ID_VIDS MKTAG('v','i','d','s')
#define ID_AUDS MKTAG('a','u','d','s')
#define ID_MIDS MKTAG('m','i','d','s')
#define ID_TXTS MKTAG('t','x','t','s')
#define ID_JUNK MKTAG('J','U','N','K')
#define ID_STRF MKTAG('s','t','r','f')
#define ID_MOVI MKTAG('m','o','v','i')
#define ID_REC  MKTAG('r','e','c',' ')
#define ID_IDX1 MKTAG('i','d','x','1')
#define ID_INFO MKTAG('I','N','F','O')
#define ID_PRMI MKTAG('P','R','M','I')

// Stream Types
enum {
	kStreamTypePaletteChange = MKTAG16('p', 'c'),
	kStreamTypeAudio         = MKTAG16('w', 'b')
};


AVIDecoder::AVIDecoder(Audio::Mixer::SoundType soundType) : _frameRateOverride(0), _soundType(soundType) {
	initCommon();
}

AVIDecoder::AVIDecoder(const Common::Rational &frameRateOverride, Audio::Mixer::SoundType soundType)
		: _frameRateOverride(frameRateOverride), _soundType(soundType) {
	initCommon();
}

AVIDecoder::~AVIDecoder() {
	close();
}

AVIDecoder::AVIAudioTrack *AVIDecoder::createAudioTrack(AVIStreamHeader sHeader, WaveFormat wvInfo, Common::SeekableReadStream *extraData) {
	return new AVIAudioTrack(sHeader, wvInfo, _soundType, extraData);
}

void AVIDecoder::initCommon() {
	_decodedHeader = false;
	_foundMovieList = false;
	_movieListStart = 0;
	_movieListEnd = 0;
	_fileStream = 0;
	memset(&_header, 0, sizeof(_header));
}

bool AVIDecoder::isSeekable() const {
	// Only videos with an index can seek
	// Anyone else who wants to seek is crazy.
	return isVideoLoaded() && !_indexEntries.empty();
}

bool AVIDecoder::parseNextChunk() {
	uint32 tag = _fileStream->readUint32BE();
	uint32 size = _fileStream->readUint32LE();

	if (_fileStream->eos())
		return false;

	debug(3, "Decoding tag %s", tag2str(tag));

	switch (tag) {
	case ID_LIST:
		handleList(size);
		break;
	case ID_AVIH:
		_header.size = size;
		_header.microSecondsPerFrame = _fileStream->readUint32LE();
		_header.maxBytesPerSecond = _fileStream->readUint32LE();
		_header.padding = _fileStream->readUint32LE();
		_header.flags = _fileStream->readUint32LE();
		_header.totalFrames = _fileStream->readUint32LE();
		_header.initialFrames = _fileStream->readUint32LE();
		_header.streams = _fileStream->readUint32LE();
		_header.bufferSize = _fileStream->readUint32LE();
		_header.width = _fileStream->readUint32LE();
		_header.height = _fileStream->readUint32LE();
		// Ignore 16 bytes of reserved data
		_fileStream->skip(16);
		break;
	case ID_STRH:
		handleStreamHeader(size);
		break;
	case ID_IDX1:
		readOldIndex(size);
		break;
	default:
		// There are many other chunk types that are useless.
		// Skip them all here.
		debug(4, "Skipping AVI chunk: \'%s\'", tag2str(tag));
		skipChunk(size);
		break;
	}

	return true;
}

void AVIDecoder::skipChunk(uint32 size) {
	// Make sure we're aligned on a word boundary
	_fileStream->skip(size + (size & 1));
}

void AVIDecoder::handleList(uint32 listSize) {
	uint32 listType = _fileStream->readUint32BE();
	listSize -= 4; // Subtract away listType's 4 bytes
	uint32 curPos = _fileStream->pos();

	debug(0, "Found LIST of type %s", tag2str(listType));

	switch (listType) {
	case ID_MOVI: // Movie List
		// We found the movie block
		_foundMovieList = true;
		_movieListStart = curPos;
		_movieListEnd = _movieListStart + listSize + (listSize & 1);
		_fileStream->skip(listSize);
		return;
	case ID_HDRL: // Header List
		// Mark the header as decoded
		_decodedHeader = true;
		break;
	case ID_INFO: // Metadata
	case ID_PRMI: // Adobe Premiere metadata, safe to ignore
		// Ignore metadata
		_fileStream->skip(listSize);
		return;
	case ID_STRL: // Stream list
	default:      // (Just hope we can parse it!)
		break;
	}

	while ((_fileStream->pos() - curPos) < listSize)
		parseNextChunk();
}

void AVIDecoder::handleStreamHeader(uint32 size) {
	AVIStreamHeader sHeader;
	sHeader.size = size;
	sHeader.streamType = _fileStream->readUint32BE();

	if (sHeader.streamType == ID_MIDS || sHeader.streamType == ID_TXTS)
		error("Unhandled MIDI/Text stream");

	sHeader.streamHandler = _fileStream->readUint32BE();
	sHeader.flags = _fileStream->readUint32LE();
	sHeader.priority = _fileStream->readUint16LE();
	sHeader.language = _fileStream->readUint16LE();
	sHeader.initialFrames = _fileStream->readUint32LE();
	sHeader.scale = _fileStream->readUint32LE();
	sHeader.rate = _fileStream->readUint32LE();
	sHeader.start = _fileStream->readUint32LE();
	sHeader.length = _fileStream->readUint32LE();
	sHeader.bufferSize = _fileStream->readUint32LE();
	sHeader.quality = _fileStream->readUint32LE();
	sHeader.sampleSize = _fileStream->readUint32LE();

	_fileStream->skip(sHeader.size - 48); // Skip over the remainder of the chunk (frame)

	if (_fileStream->readUint32BE() != ID_STRF)
		error("Could not find STRF tag");

	uint32 strfSize = _fileStream->readUint32LE();
	uint32 startPos = _fileStream->pos();

	if (sHeader.streamType == ID_VIDS) {
		if (_frameRateOverride != 0) {
			sHeader.rate = _frameRateOverride.getNumerator();
			sHeader.scale = _frameRateOverride.getDenominator();
		}

		BitmapInfoHeader bmInfo;
		bmInfo.size = _fileStream->readUint32LE();
		bmInfo.width = _fileStream->readUint32LE();
		bmInfo.height = _fileStream->readUint32LE();
		bmInfo.planes = _fileStream->readUint16LE();
		bmInfo.bitCount = _fileStream->readUint16LE();
		bmInfo.compression = _fileStream->readUint32BE();
		bmInfo.sizeImage = _fileStream->readUint32LE();
		bmInfo.xPelsPerMeter = _fileStream->readUint32LE();
		bmInfo.yPelsPerMeter = _fileStream->readUint32LE();
		bmInfo.clrUsed = _fileStream->readUint32LE();
		bmInfo.clrImportant = _fileStream->readUint32LE();

		if (bmInfo.clrUsed == 0)
			bmInfo.clrUsed = 256;

		byte *initialPalette = 0;

		if (bmInfo.bitCount == 8) {
			initialPalette = new byte[256 * 3];
			memset(initialPalette, 0, 256 * 3);

			byte *palette = initialPalette;
			for (uint32 i = 0; i < bmInfo.clrUsed; i++) {
				palette[i * 3 + 2] = _fileStream->readByte();
				palette[i * 3 + 1] = _fileStream->readByte();
				palette[i * 3] = _fileStream->readByte();
				_fileStream->readByte();
			}
		}

		// Read the rest in as extra data
		Common::SeekableReadStream *extraData = 0;

		if ((uint32)_fileStream->pos() < startPos + strfSize)
			extraData = _fileStream->readStream(strfSize - (_fileStream->pos() - startPos));

		addTrack(new AVIVideoTrack(_header.totalFrames, sHeader, bmInfo, initialPalette, extraData));
	} else if (sHeader.streamType == ID_AUDS) {
		WaveFormat wvInfo;
		wvInfo.tag = _fileStream->readUint16LE();
		wvInfo.channels = _fileStream->readUint16LE();
		wvInfo.samplesPerSec = _fileStream->readUint32LE();
		wvInfo.avgBytesPerSec = _fileStream->readUint32LE();
		wvInfo.blockAlign = _fileStream->readUint16LE();

		if (strfSize >= 16)
			wvInfo.bitsPerSample = _fileStream->readUint16LE();
		else
			wvInfo.bitsPerSample = 8;

		// TODO: Handle extended wave format
		Common::SeekableReadStream *extraData = 0;
		if (strfSize >= 18) {
			uint16 cbSize = _fileStream->readUint16LE();

			// Clamp it to the remaining strf size
			cbSize = MIN<uint16>(cbSize, strfSize - 18);

			// Read it in as extra data
			extraData = _fileStream->readStream(cbSize);
		}

		AVIAudioTrack *track = createAudioTrack(sHeader, wvInfo, extraData);
		track->createAudioStream();
		addTrack(track);
	}

	// Ensure that we're at the end of the chunk
	_fileStream->seek(startPos + strfSize + (strfSize & 1));
}

bool AVIDecoder::loadStream(Common::SeekableReadStream *stream) {
	close();

	uint32 riffTag = stream->readUint32BE();
	if (riffTag != ID_RIFF) {
		warning("Failed to find RIFF header");
		return false;
	}

	/* uint32 fileSize = */ stream->readUint32LE();
	uint32 riffType = stream->readUint32BE();

	if (riffType != ID_AVI) {
		warning("RIFF not an AVI file");
		return false;
	}

	_fileStream = stream;

	// Go through all chunks in the file
	while (parseNextChunk())
		;

	if (!_decodedHeader) {
		warning("Failed to parse AVI header");
		close();
		return false;
	}

	if (!_foundMovieList) {
		warning("Failed to find 'MOVI' list");
		close();
		return false;
	}

	// Create the status entries
	uint32 index = 0;
	for (TrackListIterator it = getTrackListBegin(); it != getTrackListEnd(); it++, index++) {
		TrackStatus status;
		status.track = *it;
		status.index = index;
		status.chunkSearchOffset = _movieListStart;

		if ((*it)->getTrackType() == Track::kTrackTypeVideo)
			_videoTracks.push_back(status);
		else
			_audioTracks.push_back(status);
	}

	if (_videoTracks.size() != 1) {
		warning("Unhandled AVI video track count: %d", _videoTracks.size());
		close();
		return false;
	}

	// Check if this is a special Duck Truemotion video
	checkTruemotion1();

	return true;
}

void AVIDecoder::close() {
	VideoDecoder::close();

	delete _fileStream;
	_fileStream = 0;
	_decodedHeader = false;
	_foundMovieList = false;
	_movieListStart = 0;
	_movieListEnd = 0;

	_indexEntries.clear();
	memset(&_header, 0, sizeof(_header));

	_videoTracks.clear();
	_audioTracks.clear();
}

void AVIDecoder::readNextPacket() {
	// Shouldn't get this unless called on a non-open video
	if (_videoTracks.empty())
		return;

	// Get the video frame first
	handleNextPacket(_videoTracks[0]);

	// Handle audio tracks next
	for (uint32 i = 0; i < _audioTracks.size(); i++)
		handleNextPacket(_audioTracks[i]);
}

void AVIDecoder::handleNextPacket(TrackStatus &status) {
	// If there's no more to search, bail out
	if (status.chunkSearchOffset + 8 >= _movieListEnd) {
		if (status.track->getTrackType() == Track::kTrackTypeVideo) {
			// Horrible AVI video has a premature end
			// Force the frame to be the last frame
			debug(0, "Forcing end of AVI video");
			((AVIVideoTrack *)status.track)->forceTrackEnd();
		}

		return;
	}

	// See if audio needs to be buffered and break out if not
	if (status.track->getTrackType() == Track::kTrackTypeAudio && !shouldQueueAudio(status))
		return;

	// Seek to where we shall start searching
	_fileStream->seek(status.chunkSearchOffset);

	for (;;) {
		// If there's no more to search, bail out
		if ((uint32)_fileStream->pos() + 8 >= _movieListEnd) {
			if (status.track->getTrackType() == Track::kTrackTypeVideo) {
				// Horrible AVI video has a premature end
				// Force the frame to be the last frame
				debug(0, "Forcing end of AVI video");
				((AVIVideoTrack *)status.track)->forceTrackEnd();
			}

			break;
		}

		uint32 nextTag = _fileStream->readUint32BE();
		uint32 size = _fileStream->readUint32LE();

		if (nextTag == ID_LIST) {
			// A list of audio/video chunks
			if (_fileStream->readUint32BE() != ID_REC)
				error("Expected 'rec ' LIST");

			continue;
		} else if (nextTag == ID_JUNK || nextTag == ID_IDX1) {
			skipChunk(size);
			continue;
		}

		// Only accept chunks for this stream
		uint32 streamIndex = getStreamIndex(nextTag);
		if (streamIndex != status.index) {
			skipChunk(size);
			continue;
		}

		Common::SeekableReadStream *chunk = 0;

		if (size != 0) {
			chunk = _fileStream->readStream(size);
			_fileStream->skip(size & 1);
		}

		if (status.track->getTrackType() == Track::kTrackTypeAudio) {
			if (getStreamType(nextTag) != kStreamTypeAudio)
				error("Invalid audio track tag '%s'", tag2str(nextTag));

			((AVIAudioTrack *)status.track)->queueSound(chunk);

			// Break out if we have enough audio
			if (!shouldQueueAudio(status))
				break;
		} else {
			AVIVideoTrack *videoTrack = (AVIVideoTrack *)status.track;

			if (getStreamType(nextTag) == kStreamTypePaletteChange) {
				// Palette Change
				videoTrack->loadPaletteFromChunk(chunk);
			} else {
				// Otherwise, assume it's a compressed frame
				videoTrack->decodeFrame(chunk);
				break;
			}
		}
	}

	// Start us off in this position next time
	status.chunkSearchOffset = _fileStream->pos();
}

bool AVIDecoder::shouldQueueAudio(TrackStatus& status) {
	// Sanity check:
	if (status.track->getTrackType() != Track::kTrackTypeAudio)
		return false;

	// If video is done, make sure that the rest of the audio is queued
	// (I guess this is also really a sanity check)
	AVIVideoTrack *videoTrack = (AVIVideoTrack *)_videoTracks[0].track;
	if (videoTrack->endOfTrack())
		return true;

	AVIAudioTrack *audioTrack = (AVIAudioTrack *)status.track;
	const AVIStreamHeader &audioHeader = audioTrack->getStreamHeader();
	const AVIStreamHeader &videoHeader = videoTrack->getStreamHeader();
	Common::Timestamp videoTimestamp(0, videoTrack->getCurFrame(), Common::Rational(videoHeader.rate, videoHeader.scale));

	Common::Timestamp audioTimestamp;
	if (audioHeader.sampleSize == 0) {
		// Variable bitrate
		audioTimestamp = Common::Timestamp(0, audioTrack->getCurChunk(), Common::Rational(audioHeader.rate, audioHeader.scale));
	} else {
		// Constant bitrate
		audioTimestamp = Common::Timestamp(0, audioTrack->getCurChunk(), Common::Rational(videoHeader.rate, videoHeader.scale));
	}

	// Add 0.5s to the video timestamp so we get some extra audio queued
	videoTimestamp = videoTimestamp.addMsecs(500);

	return audioTimestamp < videoTimestamp;
}

bool AVIDecoder::rewind() {
	if (!VideoDecoder::rewind())
		return false;

	for (uint32 i = 0; i < _videoTracks.size(); i++)
		_videoTracks[i].chunkSearchOffset = _movieListStart;

	for (uint32 i = 0; i < _audioTracks.size(); i++)
		_audioTracks[i].chunkSearchOffset = _movieListStart;

	return true;
}

bool AVIDecoder::seekIntern(const Common::Timestamp &time) {
	// Can't seek beyond the end
	if (time > getDuration())
		return false;

	// Get our video
	AVIVideoTrack *videoTrack = (AVIVideoTrack *)_videoTracks[0].track;

	// If we seek directly to the end, just mark the tracks as over
	if (time == getDuration()) {
		videoTrack->setCurFrame(videoTrack->getFrameCount() - 1);

		for (TrackListIterator it = getTrackListBegin(); it != getTrackListEnd(); it++)
			if ((*it)->getTrackType() == Track::kTrackTypeAudio)
				((AVIAudioTrack *)*it)->resetStream();

		return true;
	}

	// Seek all the video tracks
	for (uint32 i = 0; i < _videoTracks.size(); i++)
		if (!seekTrackToTime(_videoTracks[i], time))
			return false;

	// Seek all audio tracks
	for (uint32 i = 0; i < _audioTracks.size(); i++)
		if (!seekTrackToTime(_audioTracks[i], time))
			return false;

	return true;
}

bool AVIDecoder::seekTrackToTime(TrackStatus &status, const Common::Timestamp &time) {
	Track *track = status.track;

	// Figure out the frame rate of the track
	const AVIStreamHeader *streamHeader;
	if (track->getTrackType() == Track::kTrackTypeAudio)
		streamHeader = &((AVIAudioTrack *)track)->getStreamHeader();
	else
		streamHeader = &((AVIVideoTrack *)track)->getStreamHeader();

	// If it's a CBR audio track, take the rate from the video track. Otherwise,
	// use the audio track's rate.
	Common::Rational frameRate;
	if (track->getTrackType() == Track::kTrackTypeAudio && streamHeader->sampleSize != 0) {
		const AVIStreamHeader &videoStreamHeader = ((AVIVideoTrack *)_videoTracks[0].track)->getStreamHeader();
		frameRate = Common::Rational(videoStreamHeader.rate, videoStreamHeader.scale);
	} else {
		// Calculate the frame from the time and the frame rate
		frameRate = Common::Rational(streamHeader->rate, streamHeader->scale);
	}

	uint32 frame;
	if (frameRate == time.framerate())
		frame = time.totalNumberOfFrames();
	else
		frame = (Common::Rational(time.totalNumberOfFrames(), time.framerate()) * frameRate).toInt();

	// Reset any palette, if necessary
	if (track->getTrackType() == Track::kTrackTypeVideo)
		((AVIVideoTrack *)track)->useInitialPalette();

	int lastKeyFrame = -1;
	int frameIndex = -1;
	uint curFrame = 0;

	// Go through and figure out where we should be
	// If there's a palette, we need to find the palette too
	for (uint32 i = 0; i < _indexEntries.size(); i++) {
		const OldIndex &index = _indexEntries[i];

		// We don't care about RECs
		if (index.id == ID_REC)
			continue;

		// We're only looking at entries for this track
		if (getStreamIndex(index.id) != status.index)
			continue;

		uint16 streamType = getStreamType(index.id);

		if (streamType == kStreamTypePaletteChange) {
			// We need to handle any palette change we see since there's no
			// flag to tell if this is a "key" palette.

			// Only look at this for videos
			if (track->getTrackType() != Track::kTrackTypeVideo) {
				warning("Audio track with a palette change");
				continue;
			}

			// Decode the palette
			_fileStream->seek(_indexEntries[i].offset + 8);
			Common::SeekableReadStream *chunk = 0;

			if (_indexEntries[i].size != 0)
				chunk = _fileStream->readStream(_indexEntries[i].size);

			((AVIVideoTrack *)track)->loadPaletteFromChunk(chunk);
		} else {
			// Check to see if this is a keyframe
			// The first frame has to be a keyframe
			if ((_indexEntries[i].flags & AVIIF_INDEX) || curFrame == 0)
				lastKeyFrame = i;

			// Did we find the target frame?
			if (frame == curFrame) {
				frameIndex = i;
				break;
			}

			curFrame++;
		}
	}

	if (frameIndex < 0) // This shouldn't happen.
		return false;

	// For video, we want to not actually draw the frame. For audio, we do, so we can
	// skip audio to get as close to the requested time as possible.
	if (track->getTrackType() == Track::kTrackTypeAudio && (uint32)frameIndex < _indexEntries.size())
		frameIndex++;

	// Decode from keyFrame to curFrame - 1
	for (int i = lastKeyFrame; i < frameIndex; i++) {
		if (_indexEntries[i].id == ID_REC)
			continue;

		if (getStreamIndex(_indexEntries[i].id) != status.index)
			continue;

		uint16 streamType = getStreamType(_indexEntries[i].id);

		// Ignore palettes, they were already handled
		if (streamType == kStreamTypePaletteChange)
			continue;

		// Frame, hopefully
		_fileStream->seek(_indexEntries[i].offset + 8);
		Common::SeekableReadStream *chunk = 0;

		if (_indexEntries[i].size != 0)
			chunk = _fileStream->readStream(_indexEntries[i].size);

		if (track->getTrackType() == Track::kTrackTypeAudio)
			((AVIAudioTrack *)track)->queueSound(chunk);
		else
			((AVIVideoTrack *)track)->decodeFrame(chunk);
	}

	if (track->getTrackType() == Track::kTrackTypeAudio) {
		// Set the current audio chunk
		((AVIAudioTrack *)track)->setCurChunk(frame);

		// Figure out the time of the audio decoded
		Common::Timestamp decodedAudioTime(0, frame, frameRate);

		// Skip excees audio to bring us to the right time
		((AVIAudioTrack *)track)->skipAudio(time, decodedAudioTime);

		// Start audio off at the beginning of the next audio chunk
		status.chunkSearchOffset = ((uint32)frameIndex == _indexEntries.size() - 1) ? _movieListEnd : _indexEntries[frameIndex + 1].offset;
	} else {
		// Set the video track's frame
		((AVIVideoTrack *)track)->setCurFrame((int)frame - 1);

		// Start video off at the beginning of the frame needed to be decoded
		status.chunkSearchOffset = _indexEntries[frameIndex].offset;
	}

	return true;
}

byte AVIDecoder::getStreamIndex(uint32 tag) const {
	char string[3];
	WRITE_BE_UINT16(string, tag >> 16);
	string[2] = 0;
	return strtol(string, 0, 16);
}

static bool isValidStreamIndex(uint32 tag) {
	return Common::isDigit(tag >> 24) && Common::isDigit((tag >> 16) & 0xFF);
}

void AVIDecoder::readOldIndex(uint32 size) {
	uint32 entryCount = size / 16;

	debug(0, "Old Index: %d entries", entryCount);

	if (entryCount == 0)
		return;

	// Check out the first entry's offset
	uint32 startPos = _fileStream->pos();
	_fileStream->seek(8, SEEK_CUR);
	uint32 firstOffset = _fileStream->readUint32LE();
	_fileStream->seek(startPos);

	// Check if the offset is already absolute
	// If it's absolute, the offset will equal the start of the movie list
	bool isAbsolute = firstOffset == _movieListStart;

	debug(1, "Old index is %s", isAbsolute ? "absolute" : "relative");

	// Keep track of whether or not a track has a keyframe
	Common::Array<bool> hasKeyFrames;

	for (uint32 i = 0; i < entryCount; i++) {
		OldIndex indexEntry;
		indexEntry.id = _fileStream->readUint32BE();
		indexEntry.flags = _fileStream->readUint32LE();
		indexEntry.offset = _fileStream->readUint32LE();
		indexEntry.size = _fileStream->readUint32LE();

		// Adjust to absolute, if necessary
		if (!isAbsolute)
			indexEntry.offset += _movieListStart - 4;

		debug(0, "Index %d: Tag '%s', Offset = %d, Size = %d (Flags = %d)", i, tag2str(indexEntry.id), indexEntry.offset, indexEntry.size, indexEntry.flags);

		// If this was a keyframe, mark it off that we have it
		if (isValidStreamIndex(indexEntry.id)) {
			byte streamIndex = getStreamIndex(indexEntry.id);

			if (streamIndex >= hasKeyFrames.size())
				hasKeyFrames.resize(streamIndex + 1);

			if (indexEntry.flags & AVIIF_INDEX)
				hasKeyFrames[streamIndex] = true;
		}

		_indexEntries.push_back(indexEntry);
	}

	// Update the keyframe flag if the file has omitted them for all
	// entries within a track.
	for (uint32 i = 0; i < hasKeyFrames.size(); i++) {
		if (hasKeyFrames[i])
			continue;

		debug(0, "Track %d has no keyframes; forcing all to be keyframes", i);

		for (uint32 j = 0; j < _indexEntries.size(); j++)
			if (isValidStreamIndex(_indexEntries[j].id) && getStreamIndex(_indexEntries[j].id) == i)
				_indexEntries[j].flags |= AVIIF_INDEX;
	}
}

void AVIDecoder::checkTruemotion1() {
	// If we got here from loadStream(), we know the track is valid
	assert(!_videoTracks.empty());

	TrackStatus &status = _videoTracks[0];
	AVIVideoTrack *track = (AVIVideoTrack *)status.track;

	// Ignore non-truemotion tracks
	if (!track->isTruemotion1())
		return;

	// Read the next video packet
	handleNextPacket(status);

	const Graphics::Surface *frame = track->decodeNextFrame();
	if (!frame) {
		rewind();
		return;
	}

	// Fill in the width/height based on the frame's width/height
	_header.width = frame->getWidth();
	_header.height = frame->getHeight();
	track->forceDimensions(frame->getWidth(), frame->getHeight());

	// Rewind us back to the beginning
	rewind();
}

VideoDecoder::AudioTrack *AVIDecoder::getAudioTrack(int index) {
	// AVI audio track indexes are relative to the first track
	Track *track = getTrack(index);

	if (!track || track->getTrackType() != Track::kTrackTypeAudio)
		return 0;

	return (AudioTrack *)track;
}

AVIDecoder::AVIVideoTrack::AVIVideoTrack(int frameCount, const AVIStreamHeader &streamHeader, const BitmapInfoHeader &bitmapInfoHeader, byte *initialPalette, Common::SeekableReadStream *extraData)
		: _frameCount(frameCount), _vidsHeader(streamHeader), _bmInfo(bitmapInfoHeader), _initialPalette(initialPalette), _extraData(extraData) {
	_videoCodec = createCodec();
	_lastFrame = 0;
	_curFrame = -1;

	useInitialPalette();
}

AVIDecoder::AVIVideoTrack::~AVIVideoTrack() {
	delete _videoCodec;
	delete[] _initialPalette;
	delete _extraData;
}

void AVIDecoder::AVIVideoTrack::decodeFrame(Common::SeekableReadStream *stream) {
	if (stream) {
		if (_videoCodec)
			_lastFrame = _videoCodec->decodeFrame(*stream);
	} else {
		// Empty frame
		_lastFrame = 0;
	}

	delete stream;
	_curFrame++;
}

Graphics::PixelFormat AVIDecoder::AVIVideoTrack::getPixelFormat() const {
	if (_videoCodec)
		return _videoCodec->getPixelFormat();

	return Graphics::PixelFormat();
}

void AVIDecoder::AVIVideoTrack::loadPaletteFromChunk(Common::SeekableReadStream *chunk) {
	assert(chunk);
	byte firstEntry = chunk->readByte();
	uint16 numEntries = chunk->readByte();
	chunk->readUint16LE(); // Reserved

	// 0 entries means all colors are going to be changed
	if (numEntries == 0)
		numEntries = 256;

	for (uint16 i = firstEntry; i < numEntries + firstEntry; i++) {
		_palette[i * 3] = chunk->readByte();
		_palette[i * 3 + 1] = chunk->readByte();
		_palette[i * 3 + 2] = chunk->readByte();
		chunk->readByte(); // Flags that don't serve us any purpose
	}

	delete chunk;
	_dirtyPalette = true;
}

void AVIDecoder::AVIVideoTrack::useInitialPalette() {
	_dirtyPalette = false;

	if (_initialPalette) {
		memcpy(_palette, _initialPalette, sizeof(_palette));
		_dirtyPalette = true;
	}
}

bool AVIDecoder::AVIVideoTrack::isTruemotion1() const {
	return _bmInfo.compression == MKTAG('D', 'U', 'C', 'K') || _bmInfo.compression == MKTAG('d', 'u', 'c', 'k');
}

void AVIDecoder::AVIVideoTrack::forceDimensions(uint16 width, uint16 height) {
	_bmInfo.width = width;
	_bmInfo.height = height;
}

bool AVIDecoder::AVIVideoTrack::rewind() {
	_curFrame = -1;

	useInitialPalette();

	delete _videoCodec;
	_videoCodec = createCodec();
	_lastFrame = 0;
	return true;
}

Image::Codec *AVIDecoder::AVIVideoTrack::createCodec() {
	return Image::createBitmapCodec(_bmInfo.compression, _bmInfo.width, _bmInfo.height, _bmInfo.bitCount, _extraData);
}

void AVIDecoder::AVIVideoTrack::forceTrackEnd() {
	_curFrame = _frameCount - 1;
}

const byte *AVIDecoder::AVIVideoTrack::getPalette() const {
	if (_videoCodec && _videoCodec->containsPalette())
		return _videoCodec->getPalette();

	_dirtyPalette = false;
	return _palette;
}

bool AVIDecoder::AVIVideoTrack::hasDirtyPalette() const {
	if (_videoCodec && _videoCodec->containsPalette())
		return _videoCodec->hasDirtyPalette();

	return _dirtyPalette;
}

bool AVIDecoder::AVIVideoTrack::canDither() const {
	return _videoCodec && _videoCodec->canDither(Image::Codec::kDitherTypeVFW);
}

void AVIDecoder::AVIVideoTrack::setDither(const byte *palette) {
	assert(_videoCodec);
	_videoCodec->setDither(Image::Codec::kDitherTypeVFW, palette);
}

AVIDecoder::AVIAudioTrack::AVIAudioTrack(const AVIStreamHeader &streamHeader, const WaveFormat &waveFormat, Audio::Mixer::SoundType soundType, Common::SeekableReadStream *extraData)
		: _audsHeader(streamHeader), _wvInfo(waveFormat), _soundType(soundType), _audioStream(0), _packetStream(0), _curChunk(0), _extraData(extraData) {
}

AVIDecoder::AVIAudioTrack::~AVIAudioTrack() {
	delete _audioStream;
	delete _extraData;
}

void AVIDecoder::AVIAudioTrack::queueSound(Common::SeekableReadStream *stream) {
	if (stream) {
		if (_packetStream)
			_packetStream->queuePacket(stream);
		else
			delete stream;
	}

	_curChunk++;
}

void AVIDecoder::AVIAudioTrack::skipAudio(const Common::Timestamp &time, const Common::Timestamp &frameTime) {
	Common::Timestamp timeDiff = time.convertToFramerate(_wvInfo.samplesPerSec) - frameTime.convertToFramerate(_wvInfo.samplesPerSec);
	int skipFrames = timeDiff.totalNumberOfFrames();

	if (skipFrames <= 0)
		return;

	Audio::AudioStream *audioStream = getAudioStream();
	if (!audioStream)
		return;

	skipFrames *= audioStream->getChannels();

	int16 *tempBuffer = new int16[skipFrames];
	audioStream->readBuffer(tempBuffer, skipFrames);
	delete[] tempBuffer;
}

void AVIDecoder::AVIAudioTrack::resetStream() {
	delete _audioStream;
	createAudioStream();
	_curChunk = 0;
}

bool AVIDecoder::AVIAudioTrack::rewind() {
	resetStream();
	return true;
}

// Audio Codecs
enum {
	kWaveFormatNone = 0,
	kWaveFormatPCM = 1,
	kWaveFormatMSADPCM = 2,
	kWaveFormatMSIMAADPCM = 17,
	kWaveFormatMP3 = 85,
	kWaveFormatDK3 = 98, // rogue format number
	kWaveFormatVorbis = ('V' << 8) | 'o'
};

void AVIDecoder::AVIAudioTrack::createAudioStream() {
	_packetStream = 0;

	switch (_wvInfo.tag) {
	case kWaveFormatPCM: {
		byte flags = 0;
		if (_wvInfo.bitsPerSample == 16)
			flags |= Audio::FLAG_16BITS | Audio::FLAG_LITTLE_ENDIAN;
		else
			flags |= Audio::FLAG_UNSIGNED;

		if (_wvInfo.channels == 2)
			flags |= Audio::FLAG_STEREO;

		_packetStream = Audio::makePacketizedPCMStream(_wvInfo.samplesPerSec, flags);
		break;
	}
	case kWaveFormatMSADPCM:
		_packetStream = Audio::makePacketizedADPCMStream(Audio::kADPCMMS, _wvInfo.samplesPerSec, _wvInfo.channels, _wvInfo.blockAlign);
		break;
	case kWaveFormatMSIMAADPCM:
		_packetStream = Audio::makePacketizedADPCMStream(Audio::kADPCMMSIma, _wvInfo.samplesPerSec, _wvInfo.channels, _wvInfo.blockAlign);
		break;
	case kWaveFormatDK3:
		_packetStream = Audio::makePacketizedADPCMStream(Audio::kADPCMDK3, _wvInfo.samplesPerSec, _wvInfo.channels, _wvInfo.blockAlign);
		break;
	case kWaveFormatMP3:
#ifdef USE_MAD
		_packetStream = Audio::makePacketizedMP3Stream(_wvInfo.channels, _wvInfo.samplesPerSec);
#else
		warning("AVI MP3 stream found, but no libmad support compiled in");
#endif
		break;
	case kWaveFormatVorbis:
#ifdef USE_VORBIS
		_packetStream = Audio::makePacketizedVorbisStream(*_extraData);
#else
		warning("AVI Vorbis stream found, but no libvorbis support compiled in");
#endif
		break;
	case kWaveFormatNone:
		break;
	default:
		warning("Unsupported AVI audio format %d", _wvInfo.tag);
		break;
	}

	if (_packetStream)
		_audioStream = _packetStream;
	else
		_audioStream = Audio::makeNullAudioStream();
}

AVIDecoder::TrackStatus::TrackStatus() : track(0), chunkSearchOffset(0) {
}

} // End of namespace Video
