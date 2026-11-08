import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;

public class ContactTest {

    @Test
    public void testValidContactCreation() {
        Contact contact = new Contact("1", "John", "Doe", "1234567890", "123 Main Street");
        assertEquals("John", contact.getFirstName());
    }

    @Test
    public void testInvalidContactId() {
        assertThrows(IllegalArgumentException.class, () -> {
            new Contact(null, "John", "Doe", "1234567890", "123 Main Street");
        });
    }

    @Test
    public void testInvalidFirstNameLength() {
        assertThrows(IllegalArgumentException.class, () -> {
            new Contact("1", "JohnathanLong", "Doe", "1234567890", "123 Main Street");
        });
    }

    @Test
    public void testInvalidPhoneLength() {
        assertThrows(IllegalArgumentException.class, () -> {
            new Contact("1", "John", "Doe", "12345", "123 Main Street");
        });
    }
}
