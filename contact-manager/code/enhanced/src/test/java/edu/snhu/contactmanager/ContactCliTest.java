package edu.snhu.contactmanager;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.PrintWriter;
import java.io.StringReader;
import java.io.StringWriter;
import java.util.Map;
import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;

/** Sean McKenna | CS 499 | November 7, 2026. User commands and safe error reporting. */
class ContactCliTest {
    private String run(ContactService service, String commands) throws IOException {
        StringWriter output = new StringWriter();
        new ContactCli(service, new BufferedReader(new StringReader(commands)), new PrintWriter(output)).run();
        return output.toString();
    }

    @Test void everyMenuCommandAndCancellation() throws IOException {
        ContactService service = new ContactService(new InMemoryContactRepository());
        String output = run(service, "3\n1\n1\nAlex\nRivera\n2025550101\n10 Sample Road\n2\n1\n3\n"
            + "4\n1\nTaylor\nSmith\n2025550102\n20 Example Road\n5\n1\nNO\n2\n1\n5\n1\nYES\n0\n");
        for (String expected : new String[] {"No contacts saved.", "Contact saved.", "Alex Rivera",
                "Contact updated.", "Taylor Smith", "Deletion cancelled.", "Contact deleted.", "Goodbye."}) {
            assertTrue(output.contains(expected), expected);
        }
        assertTrue(service.list().isEmpty());
    }

    @Test void invalidChoiceAndIncompleteCommandDoNotChangeState() throws IOException {
        ContactService service = new ContactService(new InMemoryContactRepository());
        String output = run(service, "x\n2\nmissing\n1\n1\nAlex\n");
        assertTrue(output.contains("Choose 0"));
        assertTrue(output.contains("Contact not found."));
        assertTrue(output.contains("incomplete command was cancelled"));
        assertTrue(service.list().isEmpty());
    }

    @Test void failedSaveDoesNotDisplaySuccessOrSensitiveException() throws IOException {
        ContactRepository failing = new ContactRepository() {
            public Map<String, Contact> load() { return Map.of(); }
            public void save(Map<String, Contact> contacts) throws IOException {
                throw new IOException("PRIVATE_CONTACT_OR_PATH");
            }
        };
        ContactService service = new ContactService(failing);
        String output = run(service, "1\n1\nAlex\nRivera\n2025550101\n10 Sample Road\n0\n");
        assertTrue(output.contains("Operation failed"));
        assertFalse(output.contains("Contact saved."));
        assertFalse(output.contains("PRIVATE_CONTACT_OR_PATH"));
        assertTrue(service.list().isEmpty());
    }

    @Test void brokenInputExitsWithoutLooping() throws IOException {
        ContactService service = new ContactService(new InMemoryContactRepository());
        BufferedReader broken = new BufferedReader(new StringReader("")) {
            @Override public String readLine() throws IOException { throw new IOException("unavailable"); }
        };
        StringWriter output = new StringWriter();
        new ContactCli(service, broken, new PrintWriter(output)).run();
        assertTrue(output.toString().contains("incomplete command was cancelled"));
        assertTrue(service.list().isEmpty());
    }
}
