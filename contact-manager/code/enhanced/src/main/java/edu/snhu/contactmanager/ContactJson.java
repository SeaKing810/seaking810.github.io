package edu.snhu.contactmanager;

import com.google.gson.Strictness;
import com.google.gson.stream.JsonReader;
import com.google.gson.stream.JsonToken;
import com.google.gson.stream.JsonWriter;
import java.io.IOException;
import java.io.StringReader;
import java.io.StringWriter;
import java.util.HashMap;
import java.util.Map;
import java.util.Set;
import java.util.TreeMap;

/**
 * Sean McKenna | CS 499 Milestone Two | November 7, 2026.
 * Explicit JSON schema using Gson's strict streaming API. Deserialization calls
 * the domain constructor instead of allowing reflection to bypass validation.
 */
final class ContactJson {
    private static final Set<String> FIELDS =
        Set.of("contactId", "firstName", "lastName", "phone", "address");

    private ContactJson() { }

    static Map<String, Contact> decode(String json) throws IOException {
        Map<String, Contact> contacts = new TreeMap<>();
        try (JsonReader reader = new JsonReader(new StringReader(json))) {
            reader.setStrictness(Strictness.STRICT);
            reader.beginArray();
            while (reader.hasNext()) {
                if (contacts.size() >= ContactRepository.MAX_CONTACTS) {
                    throw new IOException("Contact limit exceeded.");
                }
                Map<String, String> fields = new HashMap<>();
                reader.beginObject();
                while (reader.hasNext()) {
                    String name = reader.nextName();
                    if (!FIELDS.contains(name) || fields.containsKey(name)
                            || reader.peek() != JsonToken.STRING) {
                        throw new IOException("Unexpected or duplicate field or non-string value.");
                    }
                    fields.put(name, reader.nextString());
                }
                reader.endObject();
                if (!fields.keySet().equals(FIELDS)) throw new IOException("Missing contact field.");
                Contact contact = new Contact(fields.get("contactId"), fields.get("firstName"),
                    fields.get("lastName"), fields.get("phone"), fields.get("address"));
                if (contacts.putIfAbsent(contact.contactId(), contact) != null) {
                    throw new IOException("Duplicate contact ID in store.");
                }
            }
            reader.endArray();
            if (reader.peek() != JsonToken.END_DOCUMENT) throw new IOException("Trailing content.");
            return contacts;
        } catch (IllegalArgumentException | IllegalStateException ex) {
            throw new IOException("Stored contact data is invalid.", ex);
        }
    }

    static String encode(Map<String, Contact> contacts) throws IOException {
        StringWriter output = new StringWriter();
        try (JsonWriter writer = new JsonWriter(output)) {
            writer.setStrictness(Strictness.STRICT);
            writer.setIndent("  ");
            writer.beginArray();
            for (Contact c : ContactRepository.validatedCopy(contacts).values()) {
                writer.beginObject();
                writer.name("contactId").value(c.contactId());
                writer.name("firstName").value(c.firstName());
                writer.name("lastName").value(c.lastName());
                writer.name("phone").value(c.phone());
                writer.name("address").value(c.address());
                writer.endObject();
            }
            writer.endArray();
        }
        return output + System.lineSeparator();
    }
}
