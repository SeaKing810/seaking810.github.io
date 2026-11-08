package edu.snhu.contactmanager;

import org.junit.jupiter.api.Test;
import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.NullAndEmptySource;
import org.junit.jupiter.params.provider.ValueSource;
import static org.junit.jupiter.api.Assertions.*;

/** Sean McKenna | CS 499 | November 7, 2026. Domain boundaries and invalid-input tests. */
class ContactTest {
    @Test void exactBoundariesAccepted() {
        Contact c = new Contact("1234567890", "A".repeat(10), "B".repeat(10),
            "0123456789", "C".repeat(30));
        assertEquals(30, c.address().length());
        assertEquals("0123456789", c.phone());
    }

    @Test void eachLengthLimitEnforced() {
        assertThrows(IllegalArgumentException.class, () -> new Contact("X".repeat(11), "A", "B", "0123456789", "C"));
        assertThrows(IllegalArgumentException.class, () -> new Contact("1", "X".repeat(11), "B", "0123456789", "C"));
        assertThrows(IllegalArgumentException.class, () -> new Contact("1", "A", "X".repeat(11), "0123456789", "C"));
        assertThrows(IllegalArgumentException.class, () -> new Contact("1", "A", "B", "0123456789", "X".repeat(31)));
    }

    @ParameterizedTest @NullAndEmptySource
    @ValueSource(strings = {" ", "\t", " A", "A ", "A\nB", "A\u001bB", "A\u202eB"})
    void requiredFieldsRejectUnsafeText(String value) {
        assertThrows(IllegalArgumentException.class, () -> new Contact(value, "A", "B", "0123456789", "C"));
        assertThrows(IllegalArgumentException.class, () -> new Contact("1", value, "B", "0123456789", "C"));
        assertThrows(IllegalArgumentException.class, () -> new Contact("1", "A", value, "0123456789", "C"));
        assertThrows(IllegalArgumentException.class, () -> new Contact("1", "A", "B", "0123456789", value));
    }

    @ParameterizedTest @NullAndEmptySource
    @ValueSource(strings = {"123456789", "12345678901", "abcdefghij", "123-567890", "１２３４５６７８９０", "123456789 "})
    void invalidPhoneRejected(String phone) {
        assertThrows(IllegalArgumentException.class, () -> new Contact("1", "A", "B", phone, "C"));
    }

    @Test void unicodeNamesAndEscapedCharactersAccepted() {
        Contact c = new Contact("1", "Zoë", "O'Neil", "0000000000", "10 \"A\" \\ Road");
        assertEquals("Zoë", c.firstName());
        assertThrows(IllegalArgumentException.class, () -> new Contact("1", "\ud800", "B", "0123456789", "C"));
    }
}
