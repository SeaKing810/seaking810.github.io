package edu.snhu.contactmanager;

import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.nio.channels.FileChannel;
import java.nio.channels.FileLock;
import java.nio.channels.OverlappingFileLockException;
import java.nio.charset.CodingErrorAction;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.LinkOption;
import java.nio.file.NoSuchFileException;
import java.nio.file.Path;
import java.nio.file.StandardOpenOption;
import java.nio.file.attribute.BasicFileAttributes;
import java.nio.file.attribute.FileAttribute;
import java.nio.file.attribute.PosixFilePermission;
import java.nio.file.attribute.PosixFilePermissions;
import java.util.Map;
import java.util.Set;

/**
 * Sean McKenna | CS 499 Milestone Two | November 7, 2026.
 * Local JSON persistence with bounded reads, private POSIX storage where supported,
 * an exclusive process lock, same-directory temporary writes, and atomic commit.
 * The storage directory is dedicated to this application; no real data is bundled.
 */
public final class JsonFileContactRepository implements ContactRepository, AutoCloseable {
    static final int MAX_BYTES = 1_048_576;
    private static final Set<PosixFilePermission> PRIVATE_DIRECTORY =
        PosixFilePermissions.fromString("rwx------");
    private static final Set<PosixFilePermission> PRIVATE_FILE =
        PosixFilePermissions.fromString("rw-------");
    private final Path directory;
    private final Path dataFile;
    private final AtomicFileStore storage;
    private final FileChannel lockChannel;
    private final FileLock lock;
    private boolean closed;

    public JsonFileContactRepository(Path directory) throws IOException {
        this(directory, new AtomicFileStore.Nio());
    }

    JsonFileContactRepository(Path requested, AtomicFileStore storage) throws IOException {
        Path absolute = requested.toAbsolutePath().normalize();
        // Only the last directory is created, avoiding permission changes to ancestors.
        if (!Files.exists(absolute, LinkOption.NOFOLLOW_LINKS)) {
            boolean posix = Files.getFileStore(absolute.getParent()).supportsFileAttributeView("posix");
            Files.createDirectory(absolute, attributes(posix, true));
        }
        if (Files.isSymbolicLink(absolute) || !Files.isDirectory(absolute, LinkOption.NOFOLLOW_LINKS)) {
            throw new IOException("Storage must be a real directory.");
        }
        this.directory = absolute.toRealPath();
        requirePrivate(this.directory, true);
        this.dataFile = this.directory.resolve("contacts.json");
        this.storage = storage;
        Path lockFile = this.directory.resolve("contacts.lock");
        if (Files.isSymbolicLink(lockFile)) throw new IOException("A symbolic lock file is not allowed.");
        boolean posix = Files.getFileStore(this.directory).supportsFileAttributeView("posix");
        FileChannel channel = FileChannel.open(lockFile,
            Set.of(StandardOpenOption.CREATE, StandardOpenOption.WRITE, LinkOption.NOFOLLOW_LINKS),
            attributes(posix, false));
        FileLock acquired = null;
        try {
            requirePrivate(lockFile, false);
            try { acquired = channel.tryLock(); }
            catch (OverlappingFileLockException ex) {
                throw new IOException("Store is already open in another instance.", ex);
            }
            if (acquired == null) throw new IOException("Store is already open in another instance.");
        } catch (IOException | RuntimeException ex) {
            channel.close();
            throw ex;
        }
        this.lockChannel = channel;
        this.lock = acquired;
    }

    @Override
    public Map<String, Contact> load() throws IOException {
        ensureOpen();
        BasicFileAttributes attributes;
        try { attributes = Files.readAttributes(dataFile, BasicFileAttributes.class,
                    LinkOption.NOFOLLOW_LINKS); }
        catch (NoSuchFileException ex) { return Map.of(); }
        if (!attributes.isRegularFile() || attributes.size() > MAX_BYTES) {
            throw new IOException("Store is not a regular file or exceeds the size limit.");
        }
        requirePrivate(dataFile, false);
        byte[] bytes;
        try (InputStream input = Files.newInputStream(dataFile, LinkOption.NOFOLLOW_LINKS)) {
            bytes = input.readNBytes(MAX_BYTES + 1);
        }
        if (bytes.length > MAX_BYTES) throw new IOException("Store exceeds the size limit.");
        String json = StandardCharsets.UTF_8.newDecoder()
            .onMalformedInput(CodingErrorAction.REPORT)
            .onUnmappableCharacter(CodingErrorAction.REPORT).decode(ByteBuffer.wrap(bytes)).toString();
        return ContactJson.decode(json);
    }

    @Override
    public void save(Map<String, Contact> contacts) throws IOException {
        ensureOpen();
        // Refuse to overwrite newly corrupted external data even after successful startup.
        load();
        byte[] bytes = ContactJson.encode(contacts).getBytes(StandardCharsets.UTF_8);
        if (bytes.length > MAX_BYTES) throw new IOException("Snapshot exceeds the size limit.");
        boolean posix = Files.getFileStore(directory).supportsFileAttributeView("posix");
        Path temporary = Files.createTempFile(directory, ".contacts-", ".tmp", attributes(posix, false));
        try {
            storage.write(temporary, bytes);
            storage.replace(temporary, dataFile);
        } finally {
            // Cleanup failure must not report a committed save as unsuccessful.
            try { Files.deleteIfExists(temporary); }
            catch (IOException ignored) { /* Private orphan can be removed after shutdown. */ }
        }
    }

    private void ensureOpen() throws IOException {
        if (closed) throw new IOException("Repository is closed.");
    }

    private static FileAttribute<?>[] attributes(boolean posix, boolean directory) {
        return posix ? new FileAttribute<?>[] {PosixFilePermissions.asFileAttribute(
            directory ? PRIVATE_DIRECTORY : PRIVATE_FILE)} : new FileAttribute<?>[0];
    }

    private static void requirePrivate(Path path, boolean directory) throws IOException {
        if (Files.getFileStore(path).supportsFileAttributeView("posix")) {
            Set<PosixFilePermission> actual = Files.getPosixFilePermissions(path, LinkOption.NOFOLLOW_LINKS);
            Set<PosixFilePermission> permitted = directory ? PRIVATE_DIRECTORY : PRIVATE_FILE;
            if (!permitted.containsAll(actual)) {
                throw new IOException("Storage permissions must restrict access to its owner.");
            }
        }
    }

    @Override
    public void close() throws IOException {
        if (!closed) {
            closed = true;
            try { lock.release(); }
            finally { lockChannel.close(); }
        }
    }
}
