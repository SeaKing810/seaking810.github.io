package edu.snhu.contactmanager;

import java.io.IOException;
import java.nio.file.AccessDeniedException;
import java.nio.file.AtomicMoveNotSupportedException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.attribute.PosixFilePermissions;
import java.util.Map;
import java.util.TreeMap;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.io.TempDir;
import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.ValueSource;
import static org.junit.jupiter.api.Assertions.*;
import static edu.snhu.contactmanager.ContactServiceTest.sample;

/** Sean McKenna | CS 499 | November 7, 2026. Real disk integration and deterministic failure injection. */
class JsonFileContactRepositoryTest {
    @TempDir Path temporaryRoot;

    private Path store() { return temporaryRoot.resolve("store"); }
    private Path file() { return store().resolve("contacts.json"); }

    private void seed() throws IOException {
        try (JsonFileContactRepository repository = new JsonFileContactRepository(store())) {
            new ContactService(repository).add(sample("1"));
        }
    }

    @Test void addEditDeletePersistAcrossReopens() throws IOException {
        seed();
        try (JsonFileContactRepository repository = new JsonFileContactRepository(store())) {
            ContactService service = new ContactService(repository);
            assertEquals(sample("1"), service.find("1"));
            service.update("1", "Taylor", "Smith", "2025550102", "20 Example Road");
        }
        try (JsonFileContactRepository repository = new JsonFileContactRepository(store())) {
            ContactService service = new ContactService(repository);
            assertEquals("Taylor", service.find("1").firstName());
            service.delete("1");
        }
        try (JsonFileContactRepository repository = new JsonFileContactRepository(store())) {
            assertTrue(repository.load().isEmpty());
        }
    }

    @Test void unicodeAndJsonEscapingRoundTrip() throws IOException {
        Contact contact = new Contact("1", "Zoë", "O'Neil", "0000000000", "10 \"A\" \\ Road");
        try (JsonFileContactRepository repository = new JsonFileContactRepository(store())) {
            repository.save(Map.of("1", contact));
            assertEquals(contact, repository.load().get("1"));
        }
    }

    @ParameterizedTest
    @ValueSource(strings = {"", "[", "null", "{}", "[] []", "[{}]", "[null]", "/*comment*/[]",
        "[{\"contactId\":\"1\",\"contactId\":\"2\"}]", "[{\"unknown\":\"x\"}]",
        "[{\"contactId\":3}]", "[[]]"})
    void malformedStoreFailsClosedAndBytesRemain(String malformed) throws IOException {
        seed(); Files.writeString(file(), malformed);
        byte[] before = Files.readAllBytes(file());
        try (JsonFileContactRepository repository = new JsonFileContactRepository(store())) {
            assertThrows(IOException.class, () -> new ContactService(repository));
            assertThrows(IOException.class, () -> repository.save(Map.of("2", sample("2"))));
        }
        assertArrayEquals(before, Files.readAllBytes(file()));
    }

    @Test void duplicateIdsAndSemanticallyInvalidDataRejected() throws IOException {
        String valid = ContactJson.encode(Map.of("1", sample("1"))).strip();
        String object = valid.substring(1, valid.length() - 1).strip();
        assertThrows(IOException.class, () -> ContactJson.decode("[" + object + "," + object + "]"));
        assertThrows(IOException.class, () -> ContactJson.decode(valid.replace("2025550101", "abcdefghij")));
        assertThrows(IOException.class, () -> ContactJson.decode(valid.replace("Alex", " ")));
        assertThrows(IOException.class, () -> ContactJson.decode(valid.replace("\"Alex\"", "null")));
        assertThrows(IOException.class, () -> ContactJson.decode(valid.replace("\"Alex\"", "42")));
    }

    @Test void excessiveBytesAndInvalidUtf8Rejected() throws IOException {
        seed();
        Files.write(file(), new byte[JsonFileContactRepository.MAX_BYTES + 1]);
        try (JsonFileContactRepository repository = new JsonFileContactRepository(store())) {
            assertThrows(IOException.class, repository::load);
        }
        Files.write(file(), new byte[] {(byte)0xc3, (byte)0x28});
        try (JsonFileContactRepository repository = new JsonFileContactRepository(store())) {
            assertThrows(IOException.class, repository::load);
        }
    }

    @Test void recordLimitAcceptedThenRejected() throws IOException {
        Map<String, Contact> contacts = new TreeMap<>();
        for (int i = 0; i < 1000; i++) contacts.put("C" + i, sample("C" + i));
        String encoded = ContactJson.encode(contacts).strip();
        assertEquals(1000, ContactJson.decode(encoded).size());
        String extra = ContactJson.encode(Map.of("extra", sample("extra"))).strip();
        String overLimit = encoded.substring(0, encoded.length() - 1) + "," + extra.substring(1);
        assertThrows(IOException.class, () -> ContactJson.decode(overLimit));
    }

    @ParameterizedTest @ValueSource(strings = {"write", "move", "unsupported"})
    void failedCommitPreservesDiskAndMemoryAndRemovesTemporaryFile(String failure) throws IOException {
        seed(); byte[] before = Files.readAllBytes(file());
        AtomicFileStore storage = new AtomicFileStore() {
            private final AtomicFileStore actual = new AtomicFileStore.Nio();
            public void write(Path temporary, byte[] bytes) throws IOException {
                if (failure.equals("write")) {
                    Files.writeString(temporary, "partial");
                    throw new AccessDeniedException("injected write denial");
                }
                actual.write(temporary, bytes);
            }
            public void replace(Path temporary, Path destination) throws IOException {
                if (failure.equals("unsupported")) {
                    throw new AtomicMoveNotSupportedException("temporary", "destination", "injected");
                }
                throw new IOException("injected pre-commit move failure");
            }
        };
        try (JsonFileContactRepository repository = new JsonFileContactRepository(store(), storage)) {
            ContactService service = new ContactService(repository);
            assertThrows(IOException.class, () -> service.update("1", "Taylor", "Smith", "2025550102", "New address"));
            assertEquals(sample("1"), service.find("1"));
        }
        assertArrayEquals(before, Files.readAllBytes(file()));
        try (var files = Files.list(store())) {
            assertTrue(files.noneMatch(path -> path.getFileName().toString().endsWith(".tmp")));
        }
    }

    @Test void secondInstanceRejectedAndLockReleasedOnClose() throws IOException {
        try (JsonFileContactRepository first = new JsonFileContactRepository(store())) {
            assertTrue(first.load().isEmpty());
            assertThrows(IOException.class, () -> new JsonFileContactRepository(store()));
        }
        try (JsonFileContactRepository reopened = new JsonFileContactRepository(store())) {
            assertTrue(reopened.load().isEmpty());
        }
    }

    @Test void closedRepositoryRefusesOperations() throws IOException {
        JsonFileContactRepository repository = new JsonFileContactRepository(store());
        repository.close(); repository.close();
        assertThrows(IOException.class, repository::load);
        assertThrows(IOException.class, () -> repository.save(Map.of()));
    }

    @Test void fileCannotServeAsDirectory() throws IOException {
        Files.writeString(store(), "occupied");
        assertThrows(IOException.class, () -> new JsonFileContactRepository(store()));
        assertEquals("occupied", Files.readString(store()));
    }

    @Test void posixStorageIsPrivateAndLoosePermissionsRejected() throws IOException {
        org.junit.jupiter.api.Assumptions.assumeTrue(Files.getFileStore(temporaryRoot).supportsFileAttributeView("posix"));
        seed();
        assertEquals(PosixFilePermissions.fromString("rwx------"), Files.getPosixFilePermissions(store()));
        assertEquals(PosixFilePermissions.fromString("rw-------"), Files.getPosixFilePermissions(file()));
        Files.setPosixFilePermissions(file(), PosixFilePermissions.fromString("rw-r--r--"));
        try (JsonFileContactRepository repository = new JsonFileContactRepository(store())) {
            assertThrows(IOException.class, repository::load);
        }
    }

    @Test void symbolicDataFileRejectedWithoutTouchingTarget() throws IOException {
        seed(); Path target = temporaryRoot.resolve("external.json");
        Files.writeString(target, "valuable"); Files.delete(file()); Files.createSymbolicLink(file(), target);
        try (JsonFileContactRepository repository = new JsonFileContactRepository(store())) {
            assertThrows(IOException.class, repository::load);
            assertThrows(IOException.class, () -> repository.save(Map.of()));
        }
        assertEquals("valuable", Files.readString(target));
    }
}
