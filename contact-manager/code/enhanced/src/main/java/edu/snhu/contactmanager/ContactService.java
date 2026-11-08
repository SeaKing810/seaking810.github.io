package edu.snhu.contactmanager;

import java.io.IOException;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.TreeMap;

/**
 * Sean McKenna | CS 499 Milestone Two | November 7, 2026.
 * Coordinates contact operations. Memory changes only after persistence succeeds.
 * Single-threaded local application; repository implementations own storage policy.
 */
public final class ContactService {
    private final ContactRepository repository;
    private Map<String, Contact> contacts;

    public ContactService(ContactRepository repository) throws IOException {
        this.repository = Objects.requireNonNull(repository);
        contacts = ContactRepository.validatedCopy(repository.load());
    }

    public Contact find(String id) {
        Contact.requireText(id, 10, "Contact ID");
        Contact contact = contacts.get(id);
        if (contact == null) throw new IllegalArgumentException("Contact not found.");
        return contact;
    }

    public List<Contact> list() { return List.copyOf(contacts.values()); }

    public void add(Contact contact) throws IOException {
        if (contact == null) throw new IllegalArgumentException("Contact is required.");
        if (contacts.containsKey(contact.contactId())) {
            throw new IllegalArgumentException("Duplicate contact ID.");
        }
        Map<String, Contact> candidate = new TreeMap<>(contacts);
        candidate.put(contact.contactId(), contact);
        commit(candidate);
    }

    public void update(String id, String first, String last, String phone, String address)
            throws IOException {
        find(id);
        // Validate ALL replacement fields before copying or committing any state.
        Contact replacement = new Contact(id, first, last, phone, address);
        Map<String, Contact> candidate = new TreeMap<>(contacts);
        candidate.put(id, replacement);
        commit(candidate);
    }

    public void delete(String id) throws IOException {
        find(id);
        Map<String, Contact> candidate = new TreeMap<>(contacts);
        candidate.remove(id);
        commit(candidate);
    }

    private void commit(Map<String, Contact> candidate) throws IOException {
        Map<String, Contact> validated = ContactRepository.validatedCopy(candidate);
        repository.save(validated);
        contacts = validated;
    }
}
