package edu.snhu.contactmanager;

import java.io.IOException;
import java.util.List;
import java.util.Map;
import java.util.TreeMap;
import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;

/** Sean McKenna | CS 499 | November 7, 2026. Service behavior and commit-boundary tests. */
class ContactServiceTest {
    static Contact sample(String id) { return new Contact(id, "Alex", "Rivera", "2025550101", "10 Sample Road"); }

    @Test void completeCrudAndSortedList() throws IOException {
        ContactService service = new ContactService(new InMemoryContactRepository());
        assertTrue(service.list().isEmpty());
        service.add(sample("2")); service.add(sample("1"));
        assertEquals(List.of("1", "2"), service.list().stream().map(Contact::contactId).toList());
        service.update("1", "Taylor", "Rivera", "2025550102", "20 Sample Road");
        assertEquals("Taylor", service.find("1").firstName());
        assertEquals("1", service.find("1").contactId());
        service.delete("2");
        assertEquals(1, service.list().size());
    }

    @Test void duplicatesNullAndMissingIdsRejected() throws IOException {
        ContactService service = new ContactService(new InMemoryContactRepository());
        service.add(sample("1"));
        assertThrows(IllegalArgumentException.class, () -> service.add(sample("1")));
        assertThrows(IllegalArgumentException.class, () -> service.add(null));
        assertThrows(IllegalArgumentException.class, () -> service.find("missing"));
        assertThrows(IllegalArgumentException.class, () -> service.delete("missing"));
        assertThrows(IllegalArgumentException.class, () -> service.update("missing", "A", "B", "0123456789", "C"));
        assertEquals(List.of(sample("1")), service.list());
    }

    @Test void invalidLastFieldCannotPartiallyUpdateEarlierFields() throws IOException {
        InMemoryContactRepository repository = new InMemoryContactRepository();
        ContactService service = new ContactService(repository);
        service.add(sample("1"));
        assertThrows(IllegalArgumentException.class,
            () -> service.update("1", "Taylor", "Morris", "2025550199", "X".repeat(31)));
        assertEquals(sample("1"), service.find("1"));
        assertEquals(sample("1"), repository.load().get("1"));
    }

    @Test void callerCannotMutateRepositoryOrServiceSnapshot() throws IOException {
        InMemoryContactRepository repository = new InMemoryContactRepository();
        Map<String, Contact> supplied = new TreeMap<>(Map.of("1", sample("1")));
        repository.save(supplied); supplied.clear();
        ContactService service = new ContactService(repository);
        assertThrows(UnsupportedOperationException.class, () -> repository.load().clear());
        assertThrows(UnsupportedOperationException.class, () -> service.list().clear());
        assertEquals(sample("1"), service.find("1"));
    }

    @Test void failuresPreserveStateForAllMutations() throws IOException {
        ContactRepository failing = new ContactRepository() {
            public Map<String, Contact> load() { return Map.of("1", sample("1")); }
            public void save(Map<String, Contact> contacts) throws IOException { throw new IOException("injected"); }
        };
        ContactService service = new ContactService(failing);
        assertThrows(IOException.class, () -> service.add(sample("2")));
        assertThrows(IOException.class, () -> service.delete("1"));
        assertThrows(IOException.class, () -> service.update("1", "Taylor", "B", "2025550102", "C"));
        assertEquals(List.of(sample("1")), service.list());
    }

    @Test void capacityAndKeyIntegrityEnforced() throws IOException {
        Map<String, Contact> full = new TreeMap<>();
        for (int i = 0; i < ContactRepository.MAX_CONTACTS; i++) full.put("C" + i, sample("C" + i));
        InMemoryContactRepository repository = new InMemoryContactRepository();
        repository.save(full);
        ContactService service = new ContactService(repository);
        assertThrows(IllegalArgumentException.class, () -> service.add(sample("extra")));
        assertEquals(1000, service.list().size());
        assertThrows(IllegalArgumentException.class, () -> repository.save(Map.of("wrong", sample("1"))));
        assertEquals(1000, repository.load().size());
    }
}
