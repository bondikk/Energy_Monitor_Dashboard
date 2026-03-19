from pathlib import Path
import sys
import unittest
from uuid import uuid4

from passlib.context import CryptContext
from sqlalchemy import create_engine
from sqlalchemy.orm import sessionmaker
from sqlalchemy.pool import StaticPool

ROOT = Path(__file__).resolve().parents[1]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from app.auth import hash_password, verify_password
from app.database import Base
from app.routes.auth_routes import login, register
from app.schemas import UserCreate, UserLogin


class AuthPasswordTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.engine = create_engine(
            "sqlite://",
            connect_args={"check_same_thread": False},
            poolclass=StaticPool,
        )
        cls.SessionTesting = sessionmaker(autocommit=False, autoflush=False, bind=cls.engine)

    def setUp(self):
        Base.metadata.drop_all(bind=self.engine)
        Base.metadata.create_all(bind=self.engine)
        self.db = self.SessionTesting()

    def tearDown(self):
        self.db.close()

    def test_short_password_register_and_login(self):
        payload = UserCreate(
            email=f"short-{uuid4()}@example.com",
            username=f"short-{uuid4().hex[:8]}",
            password="correct horse battery staple",
        )

        registered_user = register(payload, db=self.db)
        self.assertEqual(registered_user.email, payload.email)
        self.assertEqual(registered_user.username, payload.username)

        token = login(UserLogin(email=payload.email, password=payload.password), db=self.db)
        self.assertTrue(token.access_token)
        self.assertEqual(token.token_type, "bearer")

    def test_long_unicode_password_register_and_login(self):
        long_password = "пароль🔒" * 20
        self.assertGreater(len(long_password.encode("utf-8")), 72)

        payload = UserCreate(
            email=f"long-{uuid4()}@example.com",
            username=f"long-{uuid4().hex[:8]}",
            password=long_password,
        )

        registered_user = register(payload, db=self.db)
        self.assertEqual(registered_user.email, payload.email)

        token = login(UserLogin(email=payload.email, password=long_password), db=self.db)
        self.assertTrue(token.access_token)
        self.assertEqual(token.token_type, "bearer")

    def test_verify_password_accepts_legacy_bcrypt_hash(self):
        legacy_context = CryptContext(schemes=["bcrypt"], deprecated="auto")
        password = "legacy-password-123"
        legacy_hash = legacy_context.hash(password)

        self.assertTrue(verify_password(password, legacy_hash))
        self.assertFalse(verify_password("wrong-password", legacy_hash))
        self.assertTrue(hash_password(password).startswith("$bcrypt-sha256$"))


if __name__ == "__main__":
    unittest.main()