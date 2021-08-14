# - Find OggVorbis
# Find the OggVorbis includes and libraries
#
# Following variables are provided:
# OGGVORBIS_FOUND
#     True if OggVorbis has been found
# OGGVORBIS_INCLUDE_DIRS
#     The include directories of OggVorbis
# OGGVORBIS_LIBRARIES
#     OggVorbis library list


pkg_check_modules(OGGVORBIS vorbisfile)
