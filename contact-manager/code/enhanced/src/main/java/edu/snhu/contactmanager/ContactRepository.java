package edu.snhu.contactmanager;

import java.io.IOException;
import java.util.Map;
import java.util.TreeMap;

/**
 * Sean McKenna | CS 499 Milestone Two | November 7, 2026.
 * Persistence boundary: a save commits one complete validated snapshot or fails.
 * Immutable values and copied maps prevent aliasing across application layers.
 */
public interface ContactRepository {
    int MAX_CONTACTS = 1000;

    Map<String, Contact> load() throws IOException;
    void save(Map<String, Contact> contacts) throws IOException;

    static Map<String, Contact> validatedCopy(Map<String, Contact> contacts) {
        if (contacts == null || contacts.size() > MAX_CONTACTS) {
            throw new IllegalArgumentException("A store may contain at most 1000 contacts.");
        }
        Map<String, Contact> copy = new TreeMap<>();
        contacts.forEach((key, contact) -> {
            if (contact == null || !contact.contactId().equals(key)) {
                throw new IllegalArgumentException("Contact keys must match their IDs.");
            }
            copy.put(key, contact);
        });
        return copy;
    }
}
