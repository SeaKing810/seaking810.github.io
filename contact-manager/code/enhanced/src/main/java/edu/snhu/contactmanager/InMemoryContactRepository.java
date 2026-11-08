package edu.snhu.contactmanager;

import java.util.Map;

/**
 * Sean McKenna | CS 499 Milestone Two | November 7, 2026.
 * In-memory repository for isolated service tests without filesystem side effects.
 */
public final class InMemoryContactRepository implements ContactRepository {
    private Map<String, Contact> stored = Map.of();

    @Override
    public Map<String, Contact> load() { return Map.copyOf(stored); }

    @Override
    public void save(Map<String, Contact> contacts) {
        stored = Map.copyOf(ContactRepository.validatedCopy(contacts));
    }
}
