package edu.snhu.contactmanager;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.channels.FileChannel;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardCopyOption;
import java.nio.file.StandardOpenOption;

/**
 * Sean McKenna | CS 499 Milestone Two | November 7, 2026.
 * Small I/O seam permits deterministic write and move failure tests. The default
 * implementation never downgrades an unsupported atomic move to an ordinary move.
 */
interface AtomicFileStore {
    void write(Path temporary, byte[] bytes) throws IOException;
    void replace(Path temporary, Path destination) throws IOException;

    final class Nio implements AtomicFileStore {
        @Override
        public void write(Path temporary, byte[] bytes) throws IOException {
            try (FileChannel channel = FileChannel.open(temporary, StandardOpenOption.WRITE,
                    StandardOpenOption.TRUNCATE_EXISTING)) {
                ByteBuffer data = ByteBuffer.wrap(bytes);
                while (data.hasRemaining()) channel.write(data);
                channel.force(true);
            }
        }

        @Override
        public void replace(Path temporary, Path destination) throws IOException {
            Files.move(temporary, destination, StandardCopyOption.ATOMIC_MOVE,
                StandardCopyOption.REPLACE_EXISTING);
        }
    }
}
