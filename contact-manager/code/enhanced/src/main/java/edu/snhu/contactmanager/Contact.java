package edu.snhu.contactmanager;

/**
 * Sean McKenna | CS 499 Milestone Two | November 7, 2026.
 * Immutable replacement for the CS 320 mutable Contact. Construction validates
 * every field, so neither callers nor an unsuccessful edit can mutate live data.
 */
public record Contact(String contactId, String firstName, String lastName,
                      String phone, String address) {
    public Contact {
        requireText(contactId, 10, "Contact ID");
        requireText(firstName, 10, "First name");
        requireText(lastName, 10, "Last name");
        requireText(address, 30, "Address");
        if (phone == null || !phone.matches("[0-9]{10}")) {
            throw new IllegalArgumentException("Phone must contain exactly 10 ASCII digits.");
        }
    }

    static void requireText(String value, int maximum, String label) {
        if (value == null || value.isBlank() || value.length() > maximum
                || !value.equals(value.strip())
                || value.codePoints().anyMatch(c -> Character.isISOControl(c)
                    || Character.getType(c) == Character.FORMAT)
                || hasUnpairedSurrogate(value)) {
            throw new IllegalArgumentException(label + " must have 1-" + maximum
                + " characters, without edge spaces or control characters.");
        }
    }

    private static boolean hasUnpairedSurrogate(String value) {
        for (int i = 0; i < value.length(); i++) {
            char c = value.charAt(i);
            if (Character.isHighSurrogate(c)) {
                if (++i == value.length() || !Character.isLowSurrogate(value.charAt(i))) return true;
            } else if (Character.isLowSurrogate(c)) return true;
        }
        return false;
    }
}
