#include "AndroidMediaLibrary.h"
#define LOG_TAG "VLC/JNI/AndroidMediaLibrary"
#include "log.h"

#define FLAG_MEDIA_UPDATED_AUDIO       1 << 0
#define FLAG_MEDIA_UPDATED_AUDIO_EMPTY 1 << 1
#define FLAG_MEDIA_UPDATED_VIDEO       1 << 2
#define FLAG_MEDIA_ADDED_AUDIO         1 << 3
#define FLAG_MEDIA_ADDED_AUDIO_EMPTY   1 << 4
#define FLAG_MEDIA_ADDED_VIDEO         1 << 5

static pthread_key_t jni_env_key;
static JavaVM *myVm;

static void jni_detach_thread(void *data)
{
   myVm->DetachCurrentThread();
}

static void key_init(void)
{
    pthread_key_create(&jni_env_key, jni_detach_thread);
}

AndroidMediaLibrary::AndroidMediaLibrary(JavaVM *vm, fields *ref_fields, jobject thiz)
    : p_ml( NewMediaLibrary() ), p_fields ( ref_fields )
{
    myVm = vm;
    p_lister = std::make_shared<AndroidDeviceLister>();
    p_ml->setLogger( new AndroidMediaLibraryLogger );
    p_ml->setVerbosity(medialibrary::LogLevel::Info);
    p_DeviceListerCb = p_ml->setDeviceLister(p_lister);
    pthread_once(&key_once, key_init);
    JNIEnv *env = getEnv();
    if (env == NULL)
        return;
    if (p_fields->MediaLibrary.getWeakReferenceID)
    {
        weak_thiz = nullptr;
        jobject weak_compat = env->CallObjectMethod(thiz, p_fields->MediaLibrary.getWeakReferenceID);
        if (weak_compat)
            this->weak_compat = env->NewGlobalRef(weak_compat);
        env->DeleteLocalRef(weak_compat);
    } else
    {
        weak_thiz = p_fields->MediaLibrary.getWeakReferenceID ? nullptr : env->NewWeakGlobalRef(thiz);
        weak_compat = nullptr;
    }
}

AndroidMediaLibrary::~AndroidMediaLibrary()
{
    LOGD("AndroidMediaLibrary delete");
    pthread_key_delete(jni_env_key);
    delete p_ml;
}

void
AndroidMediaLibrary::initML(const std::string& dbPath, const std::string& thumbsPath)
{
    p_ml->initialize(dbPath, thumbsPath, this);
}

void
AndroidMediaLibrary::addDevice(std::string uuid, std::string path, bool removable)
{
    p_lister->addDevice(uuid, path, removable);
    p_DeviceListerCb->onDevicePlugged(uuid, path);
}

bool
AndroidMediaLibrary::removeDevice(std::string uuid)
{
    bool removed = p_lister->removeDevice(uuid);
    if (removed)
        p_DeviceListerCb->onDeviceUnplugged(uuid);
    return removed;
}

void
AndroidMediaLibrary::banFolder(const std::string& path)
{
    p_ml->banFolder(path);
}

void
AndroidMediaLibrary::discover(const std::string& libraryPath)
{
    p_ml->discover(libraryPath);
}

bool
AndroidMediaLibrary::isWorking()
{
    return m_nbDiscovery > 0 || m_progress > 0 && m_progress < 100;
}

void
AndroidMediaLibrary::pauseBackgroundOperations()
{
    p_ml->pauseBackgroundOperations();
}

void
AndroidMediaLibrary::setMediaUpdatedCbFlag(int flags)
{
    m_mediaUpdatedType = flags;
}

void
AndroidMediaLibrary::setMediaAddedCbFlag(int flags)
{
    m_mediaAddedType = flags;
}

void
AndroidMediaLibrary::resumeBackgroundOperations()
{
    p_ml->resumeBackgroundOperations();
}

void
AndroidMediaLibrary::reload()
{
    p_ml->reload();
}

void
AndroidMediaLibrary::reload( const std::string& entryPoint )
{
    p_ml->reload(entryPoint);
}

bool
AndroidMediaLibrary::increasePlayCount(int64_t mediaId)
{
    return p_ml->media(mediaId)->increasePlayCount();
}

bool
AndroidMediaLibrary::updateProgress(int64_t mediaId, int64_t time)
{
    medialibrary::MediaPtr media = p_ml->media(mediaId);
    if (media->duration() == 0)
        return false;
    float progress = time/(double)media->duration();
    if (progress > 0.95)
        progress = 0.0;
    LOGD("update progress %f", progress);
    return media->setProgress(progress);
}

std::vector<medialibrary::MediaPtr>
AndroidMediaLibrary::lastMediaPlayed()
{
    return p_ml->lastMediaPlayed();
}

medialibrary::SearchAggregate
AndroidMediaLibrary::search(const std::string& query)
{
    return p_ml->search(query);
}

medialibrary::MediaPtr
AndroidMediaLibrary::media(long id)
{
    return p_ml->media(id);
}

medialibrary::MediaPtr
AndroidMediaLibrary::media(const std::string& mrl)
{
    return p_ml->media(mrl);
}

std::vector<medialibrary::MediaPtr>
AndroidMediaLibrary::videoFiles( medialibrary::SortingCriteria sort, bool desc )
{
    return p_ml->videoFiles(sort, desc);
}

std::vector<medialibrary::MediaPtr>
AndroidMediaLibrary::audioFiles( medialibrary::SortingCriteria sort, bool desc )
{
    return p_ml->audioFiles(sort, desc);
}

std::vector<medialibrary::AlbumPtr>
AndroidMediaLibrary::albums()
{
    return p_ml->albums();
}

medialibrary::AlbumPtr
AndroidMediaLibrary::album(int64_t albumId)
{
    return p_ml->album(albumId);
}

std::vector<medialibrary::ArtistPtr>
AndroidMediaLibrary::artists()
{
    return p_ml->artists();
}

medialibrary::ArtistPtr
AndroidMediaLibrary::artist(int64_t artistId)
{
    return p_ml->artist(artistId);
}

std::vector<medialibrary::GenrePtr>
AndroidMediaLibrary::genres()
{
    return p_ml->genres();
}

medialibrary::GenrePtr
AndroidMediaLibrary::genre(int64_t genreId)
{
    return p_ml->genre(genreId);
}

std::vector<medialibrary::PlaylistPtr>
AndroidMediaLibrary::playlists()
{
    return p_ml->playlists();
}

medialibrary::PlaylistPtr
AndroidMediaLibrary::playlist( int64_t playlistId )
{
    return p_ml->playlist(playlistId);
}

medialibrary::PlaylistPtr
AndroidMediaLibrary::PlaylistCreate( const std::string &name )
{
    return p_ml->createPlaylist(name);
}

std::vector<medialibrary::MediaPtr>
AndroidMediaLibrary::tracksFromAlbum( int64_t albumId )
{
    return p_ml->album(albumId)->tracks();
}

std::vector<medialibrary::MediaPtr>
AndroidMediaLibrary::mediaFromArtist( int64_t artistId )
{
    return p_ml->artist(artistId)->media();
}

std::vector<medialibrary::AlbumPtr>
AndroidMediaLibrary::albumsFromArtist( int64_t artistId )
{
    return p_ml->artist(artistId)->albums();
}

std::vector<medialibrary::MediaPtr>
AndroidMediaLibrary::mediaFromGenre( int64_t genreId )
{
    return p_ml->genre(genreId)->tracks();
}

std::vector<medialibrary::AlbumPtr>
AndroidMediaLibrary::albumsFromGenre( int64_t genreId )
{
    return p_ml->genre(genreId)->albums();
}

std::vector<medialibrary::ArtistPtr>
AndroidMediaLibrary::artistsFromGenre( int64_t genreId )
{
    return p_ml->genre(genreId)->artists();
}

std::vector<medialibrary::MediaPtr>
AndroidMediaLibrary::mediaFromPlaylist( int64_t playlistId )
{
    return p_ml->playlist(playlistId)->media();
}

bool
AndroidMediaLibrary::playlistAppend(int64_t playlistId, int64_t mediaId) {
    medialibrary::PlaylistPtr playlist = p_ml->playlist(playlistId);
    if (playlist == nullptr)
        return false;
    return playlist->append(mediaId);
}

bool
AndroidMediaLibrary::playlistAdd(int64_t playlistId, int64_t mediaId, unsigned int position) {
    medialibrary::PlaylistPtr playlist = p_ml->playlist(playlistId);
    if (playlist == nullptr)
        return false;
    return playlist->add(mediaId, position);
}

bool
AndroidMediaLibrary::playlistMove(int64_t playlistId, int64_t mediaId, unsigned int position) {
    medialibrary::PlaylistPtr playlist = p_ml->playlist(playlistId);
    if (playlist == nullptr)
        return false;
    return playlist->move(mediaId, position);
}

bool
AndroidMediaLibrary::playlistRemove(int64_t playlistId, int64_t mediaId) {
    medialibrary::PlaylistPtr playlist = p_ml->playlist(playlistId);
    if (playlist == nullptr)
        return false;
    return playlist->remove(mediaId);
}

bool
AndroidMediaLibrary::PlaylistDelete( int64_t playlistId )
{
    return p_ml->deletePlaylist(playlistId);
}

void
AndroidMediaLibrary::onMediaAdded( std::vector<medialibrary::MediaPtr> mediaList )
{
    if (m_mediaAddedType & FLAG_MEDIA_ADDED_AUDIO || m_mediaAddedType & FLAG_MEDIA_ADDED_VIDEO
            || m_mediaAddedType & FLAG_MEDIA_ADDED_AUDIO_EMPTY) {
        JNIEnv *env = getEnv();
        if (env == NULL /*|| env->IsSameObject(weak_thiz, NULL)*/)
            return;
        jobjectArray mediaRefs;
        int index;
        if (m_mediaAddedType & FLAG_MEDIA_ADDED_AUDIO_EMPTY)
        {
            index = 0;
            mediaRefs = (jobjectArray) env->NewObjectArray(0, p_fields->MediaWrapper.clazz, NULL);
        } else
        {
            mediaRefs = (jobjectArray) env->NewObjectArray(mediaList.size(), p_fields->MediaWrapper.clazz, NULL);
            index = -1;
            for (medialibrary::MediaPtr const& media : mediaList) {
                medialibrary::IMedia::Type type = media->type();
                if (!((type == medialibrary::IMedia::Type::AudioType && m_mediaAddedType & FLAG_MEDIA_ADDED_AUDIO) ||
                        (type == medialibrary::IMedia::Type::VideoType && m_mediaAddedType & FLAG_MEDIA_ADDED_VIDEO)))
                    continue;
                jobject item = mediaToMediaWrapper(env, p_fields, media);
                env->SetObjectArrayElement(mediaRefs, ++index, item);
                env->DeleteLocalRef(item);
            }
        }

        if (index > -1)
        {
            jobject thiz = getWeakReference(env);
            if (thiz)
            {
                env->CallVoidMethod(thiz, p_fields->MediaLibrary.onMediaAddedId, mediaRefs);
                if (weak_compat)
                    env->DeleteLocalRef(thiz);
            }
        }
        env->DeleteLocalRef(mediaRefs);
    }
}

void AndroidMediaLibrary::onMediaUpdated( std::vector<medialibrary::MediaPtr> mediaList )
{
    if (m_mediaUpdatedType & FLAG_MEDIA_UPDATED_AUDIO || m_mediaUpdatedType & FLAG_MEDIA_UPDATED_VIDEO
            || m_mediaUpdatedType & FLAG_MEDIA_UPDATED_AUDIO_EMPTY) {
        JNIEnv *env = getEnv();
        if (env == NULL)
            return;
        jobjectArray mediaRefs;
        int index;
        if (m_mediaUpdatedType & FLAG_MEDIA_UPDATED_AUDIO_EMPTY)
        {
            index = 0;
            mediaRefs = (jobjectArray) env->NewObjectArray(0, p_fields->MediaWrapper.clazz, NULL);
        } else
        {
            mediaRefs = (jobjectArray) env->NewObjectArray(mediaList.size(), p_fields->MediaWrapper.clazz, NULL);
            index = -1;
            for (medialibrary::MediaPtr const& media : mediaList) {
                medialibrary::IMedia::Type type = media->type();
                if (!((type == medialibrary::IMedia::Type::AudioType && m_mediaUpdatedType & FLAG_MEDIA_UPDATED_AUDIO) ||
                        (type == medialibrary::IMedia::Type::VideoType && m_mediaUpdatedType & FLAG_MEDIA_UPDATED_VIDEO)))
                    continue;
                jobject item = mediaToMediaWrapper(env, p_fields, media);
                env->SetObjectArrayElement(mediaRefs, ++index, item);
                env->DeleteLocalRef(item);
            }
        }
        if (index > -1)
        {
            jobject thiz = getWeakReference(env);
            if (thiz)
            {
                env->CallVoidMethod(thiz, p_fields->MediaLibrary.onMediaUpdatedId, mediaRefs);
                if (weak_compat)
                    env->DeleteLocalRef(thiz);
            }
        }
        env->DeleteLocalRef(mediaRefs);
    }
}

void AndroidMediaLibrary::onMediaDeleted( std::vector<int64_t> ids )
{

}

void AndroidMediaLibrary::onArtistsAdded( std::vector<medialibrary::ArtistPtr> artists )
{
    if (m_mediaAddedType & FLAG_MEDIA_ADDED_AUDIO)
    {
        JNIEnv *env = getEnv();
        if (env == NULL)
            return;
        jobject thiz = getWeakReference(env);
        if (thiz)
        {
            env->CallVoidMethod(thiz, p_fields->MediaLibrary.onArtistsAddedId);
            if (weak_compat)
                env->DeleteLocalRef(thiz);
        }
    }
}

void AndroidMediaLibrary::onArtistsModified( std::vector<medialibrary::ArtistPtr> artist )
{
    if (m_mediaUpdatedType & FLAG_MEDIA_UPDATED_AUDIO)
    {
        JNIEnv *env = getEnv();
        if (env == NULL)
            return;
        jobject thiz = getWeakReference(env);
        if (thiz)
        {
            env->CallVoidMethod(thiz, p_fields->MediaLibrary.onArtistsModifiedId);
            if (weak_compat)
                env->DeleteLocalRef(thiz);
        }
    }
}

void AndroidMediaLibrary::onArtistsDeleted( std::vector<int64_t> ids )
{

}

void AndroidMediaLibrary::onAlbumsAdded( std::vector<medialibrary::AlbumPtr> albums )
{
    if (m_mediaAddedType & FLAG_MEDIA_ADDED_AUDIO)
    {
        JNIEnv *env = getEnv();
        if (env == NULL)
            return;
        jobject thiz = getWeakReference(env);
        if (thiz)
        {
            env->CallVoidMethod(thiz, p_fields->MediaLibrary.onAlbumsAddedId);
            if (weak_compat)
                env->DeleteLocalRef(thiz);
        }
    }
}

void AndroidMediaLibrary::onAlbumsModified( std::vector<medialibrary::AlbumPtr> albums )
{
    if (m_mediaUpdatedType & FLAG_MEDIA_UPDATED_AUDIO)
    {
        JNIEnv *env = getEnv();
        if (env == NULL)
            return;
        jobject thiz = getWeakReference(env);
        if (thiz)
        {
            env->CallVoidMethod(thiz, p_fields->MediaLibrary.onAlbumsModifiedId);
            if (weak_compat)
                env->DeleteLocalRef(thiz);
        }
    }
}

void AndroidMediaLibrary::onPlaylistsAdded( std::vector<medialibrary::PlaylistPtr> playlists )
{

}

void AndroidMediaLibrary::onPlaylistsModified( std::vector<medialibrary::PlaylistPtr> playlist )
{

}

void AndroidMediaLibrary::onPlaylistsDeleted( std::vector<int64_t> ids )
{

}

void AndroidMediaLibrary::onAlbumsDeleted( std::vector<int64_t> ids )
{

}

void AndroidMediaLibrary::onTracksAdded( std::vector<medialibrary::AlbumTrackPtr> tracks )
{

}
void AndroidMediaLibrary::onTracksDeleted( std::vector<int64_t> trackIds )
{

}

void AndroidMediaLibrary::onDiscoveryStarted( const std::string& entryPoint )
{
    ++m_nbDiscovery;
    JNIEnv *env = getEnv();
    if (env == NULL)
        return;
    if (mainStorage.empty()) {
        discoveryEnded = false;
        mainStorage = entryPoint;
    }
    jstring ep = env->NewStringUTF(entryPoint.c_str());
    jobject thiz = getWeakReference(env);
    if (thiz != NULL)
    {
        env->CallVoidMethod(thiz, p_fields->MediaLibrary.onDiscoveryStartedId, ep);
        if (weak_compat)
            env->DeleteLocalRef(thiz);
    }
    env->DeleteLocalRef(ep);
}

void AndroidMediaLibrary::onDiscoveryProgress( const std::string& entryPoint )
{
    JNIEnv *env = getEnv();
    if (env == NULL)
        return;
    jstring ep = env->NewStringUTF(entryPoint.c_str());
    jobject thiz = getWeakReference(env);
    if (thiz)
    {
        env->CallVoidMethod(thiz, p_fields->MediaLibrary.onDiscoveryProgressId, ep);
        if (weak_compat)
            env->DeleteLocalRef(thiz);
    }
    env->DeleteLocalRef(ep);

}

void AndroidMediaLibrary::onDiscoveryCompleted( const std::string& entryPoint )
{
    --m_nbDiscovery;
    JNIEnv *env = getEnv();
    if (env == NULL)
        return;
    if (!entryPoint.compare(mainStorage)) {
        discoveryEnded = true;
        mainStorage.clear();
    }
    jstring ep = env->NewStringUTF(entryPoint.c_str());
    jobject thiz = getWeakReference(env);
    if (thiz) {
        if (m_progress)
            env->CallVoidMethod(thiz, p_fields->MediaLibrary.onParsingStatsUpdatedId, m_progress);
        env->CallVoidMethod(thiz, p_fields->MediaLibrary.onDiscoveryCompletedId, ep);
        if (weak_compat)
            env->DeleteLocalRef(thiz);
    }
    env->DeleteLocalRef(ep);
}

void AndroidMediaLibrary::onParsingStatsUpdated( uint32_t percent)
{
    m_progress = percent;
    if (!discoveryEnded)
        return;
    JNIEnv *env = getEnv();
    if (env == NULL)
        return;
    jint progress = percent;
    jobject thiz = getWeakReference(env);
    if (thiz != NULL)
    {
        env->CallVoidMethod(thiz, p_fields->MediaLibrary.onParsingStatsUpdatedId, progress);
        if (weak_compat)
            env->DeleteLocalRef(thiz);
    }
}

jobject
AndroidMediaLibrary::getWeakReference(JNIEnv *env)
{
    return weak_thiz != nullptr ? weak_thiz : env->CallObjectMethod(weak_compat, p_fields->WeakReference.getID);
}

JNIEnv *
AndroidMediaLibrary::getEnv() {
    JNIEnv *env = (JNIEnv *)pthread_getspecific(jni_env_key);
    if (!env)
    {
        switch (myVm->GetEnv((void**)(&env), VLC_JNI_VERSION))
        {
        case JNI_OK:
            break;
        case JNI_EDETACHED:
            if (myVm->AttachCurrentThread(&env, NULL) != JNI_OK)
                return NULL;
            if (pthread_setspecific(jni_env_key, env) != 0)
            {
                detachCurrentThread();
                return NULL;
            }
            break;
        default:
            LOGE("failed to get env");
        }
    }
    return env;
}

void
AndroidMediaLibrary::detachCurrentThread() {
    myVm->DetachCurrentThread();
}
